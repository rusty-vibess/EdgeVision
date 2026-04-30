#include <gtest/gtest.h>

#include "config/types/image.hpp"
#include "streaming/webrtc/PipelineBuilder.hpp"

using edgevision::config::ImageSize;
using edgevision::config::WebRtcStreamingConfig;
using edgevision::streaming::webrtc::buildPipeline;
using edgevision::streaming::webrtc::PipelineHandles;
using edgevision::streaming::webrtc::pipelineString;

namespace {

    constexpr guint kLegacyRgbVideoMLineIndex = 0;
    constexpr guint kLegacyTsdfVideoMLineIndex = 1;
    constexpr gint kH264PayloadType = 96;
    constexpr char kH264CodecPreferencesCaps[] =
        "application/x-rtp,media=(string)video,encoding-name=(string)H264,"
        "payload=(int)96,clock-rate=(int)90000,packetization-mode=(string)1,"
        "level-asymmetry-allowed=(string)1,profile-level-id=(string)42e01f";

    void ensureGStreamerInitialized() {
        static const bool initialized = []() {
            gst_init(nullptr, nullptr);
            return true;
        }();
        (void)initialized;
    }

    [[nodiscard]] GstPad* requestSinkPad(GstElement* webrtcbin, guint mLineIndex) {
        gchar* padName = g_strdup_printf("sink_%u", mLineIndex);
        GstPad* sinkPad = gst_element_request_pad_simple(webrtcbin, padName);
        g_free(padName);
        return sinkPad;
    }

    void releaseRequestedPad(GstElement* webrtcbin, GstPad*& sinkPad) {
        if (sinkPad == nullptr) {
            return;
        }
        gst_element_release_request_pad(webrtcbin, sinkPad);
        gst_object_unref(sinkPad);
        sinkPad = nullptr;
    }

    [[nodiscard]] GstWebRTCRTPTransceiver* getPadTransceiver(GstPad* sinkPad) {
        GstWebRTCRTPTransceiver* transceiver = nullptr;
        g_object_get(sinkPad, "transceiver", &transceiver, nullptr);
        return transceiver;
    }

    [[nodiscard]] const char* directionName(GstWebRTCRTPTransceiverDirection direction) {
        switch (direction) {
            case GST_WEBRTC_RTP_TRANSCEIVER_DIRECTION_NONE:
                return "none";
            case GST_WEBRTC_RTP_TRANSCEIVER_DIRECTION_INACTIVE:
                return "inactive";
            case GST_WEBRTC_RTP_TRANSCEIVER_DIRECTION_SENDONLY:
                return "sendonly";
            case GST_WEBRTC_RTP_TRANSCEIVER_DIRECTION_RECVONLY:
                return "recvonly";
            case GST_WEBRTC_RTP_TRANSCEIVER_DIRECTION_SENDRECV:
                return "sendrecv";
            default:
                return "unknown";
        }
    }

    void logPadTransceiverState(const char* label, GstPad* sinkPad) {
        if (sinkPad == nullptr) {
            std::cerr << "WebRTC test " << label << " sink pad is null\n";
            return;
        }

        GstWebRTCRTPTransceiver* transceiver = getPadTransceiver(sinkPad);
        if (transceiver == nullptr) {
            std::cerr << "WebRTC test " << label << " sinkPad=" << sinkPad
                      << " has no associated transceiver\n";
            return;
        }

        gchar* mid = nullptr;
        gint kind = GST_WEBRTC_KIND_UNKNOWN;
        guint mlineIndex = static_cast<guint>(-1);
        GstWebRTCRTPTransceiverDirection direction = GST_WEBRTC_RTP_TRANSCEIVER_DIRECTION_NONE;
        g_object_get(
            transceiver,
            "mid",
            &mid,
            "kind",
            &kind,
            "mlineindex",
            &mlineIndex,
            "direction",
            &direction,
            nullptr
        );
        std::cerr << "WebRTC test " << label << " sinkPad=" << sinkPad
                  << " transceiver=" << transceiver << " kind=" << kind
                  << " mlineindex=" << mlineIndex << " mid=" << (mid == nullptr ? "<null>" : mid)
                  << " direction=" << directionName(direction) << '\n';
        g_free(mid);
        gst_object_unref(transceiver);
    }

    void logKnownTransceivers(GstElement* webrtcbin, const char* stage) {
        GArray* transceivers = nullptr;
        g_signal_emit_by_name(webrtcbin, "get-transceivers", &transceivers);
        if (transceivers == nullptr) {
            std::cerr << "WebRTC test " << stage << " get-transceivers returned null\n";
            return;
        }

        std::cerr << "WebRTC test " << stage << " transceiver-count=" << transceivers->len << '\n';
        for (guint index = 0; index < transceivers->len; ++index) {
            auto* transceiver = g_array_index(transceivers, GstWebRTCRTPTransceiver*, index);
            if (transceiver == nullptr) {
                std::cerr << "WebRTC test " << stage << " transceiver[" << index << "]=<null>\n";
                continue;
            }

            gchar* mid = nullptr;
            gint kind = GST_WEBRTC_KIND_UNKNOWN;
            guint mlineIndex = static_cast<guint>(-1);
            GstWebRTCRTPTransceiverDirection direction = GST_WEBRTC_RTP_TRANSCEIVER_DIRECTION_NONE;
            g_object_get(
                transceiver,
                "mid",
                &mid,
                "kind",
                &kind,
                "mlineindex",
                &mlineIndex,
                "direction",
                &direction,
                nullptr
            );
            std::cerr << "WebRTC test " << stage << " transceiver[" << index << "]=" << transceiver
                      << " kind=" << kind << " mlineindex=" << mlineIndex
                      << " mid=" << (mid == nullptr ? "<null>" : mid)
                      << " direction=" << directionName(direction) << '\n';
            g_free(mid);
        }

        g_array_unref(transceivers);
    }

    void expectPadTransceiverDirection(
        GstPad* sinkPad,
        GstWebRTCRTPTransceiverDirection expectedDirection
    ) {
        GstWebRTCRTPTransceiver* transceiver = getPadTransceiver(sinkPad);
        ASSERT_NE(transceiver, nullptr);

        GstWebRTCRTPTransceiverDirection actualDirection =
            GST_WEBRTC_RTP_TRANSCEIVER_DIRECTION_NONE;
        g_object_get(transceiver, "direction", &actualDirection, nullptr);
        EXPECT_EQ(actualDirection, expectedDirection);
        gst_object_unref(transceiver);
    }

    [[nodiscard]] bool configurePadTransceiverDirection(
        GstPad* sinkPad,
        GstWebRTCRTPTransceiverDirection direction
    ) {
        GstWebRTCRTPTransceiver* transceiver = getPadTransceiver(sinkPad);
        if (transceiver == nullptr) {
            return false;
        }

        g_object_set(transceiver, "direction", direction, nullptr);
        gst_object_unref(transceiver);
        return true;
    }

    [[nodiscard]] bool configurePadTransceiverCodecPreferences(GstPad* sinkPad) {
        GstWebRTCRTPTransceiver* transceiver = getPadTransceiver(sinkPad);
        if (transceiver == nullptr) {
            return false;
        }

        GstCaps* codecPreferences = gst_caps_from_string(kH264CodecPreferencesCaps);
        if (codecPreferences == nullptr) {
            gst_object_unref(transceiver);
            return false;
        }

        g_object_set(transceiver, "codec-preferences", codecPreferences, nullptr);
        gst_caps_unref(codecPreferences);
        gst_object_unref(transceiver);
        return true;
    }

    [[nodiscard]] bool linkPayloaderToSinkPad(GstElement* videoPayloader, GstPad* sinkPad) {
        GstPad* payloaderSrcPad = gst_element_get_static_pad(videoPayloader, "src");
        if (payloaderSrcPad == nullptr) {
            return false;
        }

        const GstPadLinkReturn linkResult = gst_pad_link(payloaderSrcPad, sinkPad);
        gst_object_unref(payloaderSrcPad);
        return linkResult == GST_PAD_LINK_OK;
    }

    [[nodiscard]] GstWebRTCDataChannel* createLocalDataChannel(GstElement* webrtcbin) {
        GstWebRTCDataChannel* channel = nullptr;
        g_signal_emit_by_name(webrtcbin, "create-data-channel", "control", nullptr, &channel);
        return channel;
    }

    [[nodiscard]] GstWebRTCSessionDescription* createLocalOffer(GstElement* webrtcbin) {
        GstPromise* promise = gst_promise_new();
        g_signal_emit_by_name(webrtcbin, "create-offer", nullptr, promise);

        const GstPromiseResult result = gst_promise_wait(promise);
        if (result != GST_PROMISE_RESULT_REPLIED) {
            gst_promise_unref(promise);
            return nullptr;
        }

        const GstStructure* reply = gst_promise_get_reply(promise);
        GstWebRTCSessionDescription* offer = nullptr;
        gst_structure_get(reply, "offer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &offer, nullptr);
        gst_promise_unref(promise);
        return offer;
    }

    [[nodiscard]] std::string toText(const GstSDPMessage* message) {
        gchar* text = gst_sdp_message_as_text(message);
        const std::string result = text == nullptr ? std::string{} : std::string{text};
        g_free(text);
        return result;
    }

    [[nodiscard]] std::string toText(const GstSDPMedia* media) {
        gchar* text = gst_sdp_media_as_text(media);
        const std::string result = text == nullptr ? std::string{} : std::string{text};
        g_free(text);
        return result;
    }

    [[nodiscard]] bool startsWith(const std::string_view value, const std::string_view prefix) {
        return value.substr(0, prefix.size()) == prefix;
    }

    void destroyPipelineHandles(PipelineHandles& handles) {
        if (handles.pipeline != nullptr) {
            gst_element_set_state(handles.pipeline, GST_STATE_NULL);
        }
        if (handles.videoPayloader != nullptr) {
            gst_object_unref(handles.videoPayloader);
            handles.videoPayloader = nullptr;
        }
        if (handles.videoAppSrc != nullptr) {
            gst_object_unref(handles.videoAppSrc);
            handles.videoAppSrc = nullptr;
        }
        if (handles.webrtcbin != nullptr) {
            gst_object_unref(handles.webrtcbin);
            handles.webrtcbin = nullptr;
        }
        if (handles.pipeline != nullptr) {
            gst_object_unref(handles.pipeline);
            handles.pipeline = nullptr;
        }
    }

} // namespace

TEST(PipelineBuilder, ContainsViewerOutputBranchAndWebrtcbin) {
    WebRtcStreamingConfig cfg;
    const ImageSize imageSize{854, 480};
    cfg.fps = 30;
    cfg.bitrateBps = 1500000;
    const auto s = pipelineString(cfg, imageSize);
    EXPECT_NE(s.find("appsrc name=src_video"), std::string::npos);
    EXPECT_NE(s.find("webrtcbin name=sendrecv"), std::string::npos);
    EXPECT_NE(s.find("format=RGB"), std::string::npos);
    EXPECT_NE(s.find("x264enc"), std::string::npos);
    EXPECT_NE(s.find("video/x-h264,profile=constrained-baseline"), std::string::npos);
    EXPECT_NE(
        s.find("video/x-h264,profile=constrained-baseline,stream-format=byte-stream,alignment=au"),
        std::string::npos
    );
    EXPECT_NE(s.find("rtph264pay name=pay_video"), std::string::npos);
    EXPECT_NE(s.find("bitrate=1500"), std::string::npos);
    EXPECT_NE(s.find("tune=zerolatency"), std::string::npos);
    EXPECT_NE(s.find("threads=2"), std::string::npos);
    EXPECT_EQ(s.find("stun-server="), std::string::npos);
    EXPECT_EQ(s.find("sendrecv.sink_1"), std::string::npos);
}

TEST(PipelineBuilder, IncludesConfiguredStunServer) {
    WebRtcStreamingConfig cfg;
    cfg.stunServer = "stun://example.invalid:19302";
    const auto s = pipelineString(cfg, ImageSize{1280, 720});
    EXPECT_NE(s.find("stun-server=stun://example.invalid:19302"), std::string::npos);
}

TEST(PipelineBuilder, OmitsLegacyBranchNames) {
    WebRtcStreamingConfig cfg;
    const auto s = pipelineString(cfg, ImageSize{1280, 720});
    EXPECT_EQ(s.find("appsrc name=src_rgb"), std::string::npos);
    EXPECT_EQ(s.find("appsrc name=src_tsdf"), std::string::npos);
}

TEST(PipelineBuilder, FrameRateAndIdrInterval) {
    WebRtcStreamingConfig cfg;
    cfg.fps = 30;
    const auto s = pipelineString(cfg, ImageSize{1280, 720});
    EXPECT_NE(s.find("framerate=30/1"), std::string::npos);
    EXPECT_NE(s.find("key-int-max=30"), std::string::npos);
}

TEST(PipelineBuilder, CreatesServerOfferWithSingleVideoSlot) {
    ensureGStreamerInitialized();

    WebRtcStreamingConfig cfg;
    PipelineHandles handles = buildPipeline(cfg, ImageSize{1280, 720});

    ASSERT_NE(handles.pipeline, nullptr);
    ASSERT_NE(handles.webrtcbin, nullptr);
    ASSERT_NE(handles.videoAppSrc, nullptr);
    ASSERT_NE(handles.videoPayloader, nullptr);

    const GstStateChangeReturn stateChange =
        gst_element_set_state(handles.pipeline, GST_STATE_READY);
    ASSERT_NE(stateChange, GST_STATE_CHANGE_FAILURE);

    GstPad* activeSinkPad = requestSinkPad(handles.webrtcbin, kLegacyRgbVideoMLineIndex);
    ASSERT_NE(activeSinkPad, nullptr) << "failed to request sink_0";
    ASSERT_TRUE(configurePadTransceiverCodecPreferences(activeSinkPad))
        << "failed to set H264 codec preferences on sink_0";
    ASSERT_TRUE(configurePadTransceiverDirection(
        activeSinkPad, GST_WEBRTC_RTP_TRANSCEIVER_DIRECTION_SENDONLY
    ));
    expectPadTransceiverDirection(activeSinkPad, GST_WEBRTC_RTP_TRANSCEIVER_DIRECTION_SENDONLY);

    ASSERT_TRUE(linkPayloaderToSinkPad(handles.videoPayloader, activeSinkPad))
        << "failed to link pay_video to sink_0";
    logPadTransceiverState("single-video sink_0", activeSinkPad);
    logKnownTransceivers(handles.webrtcbin, "single-video before create-offer");

    GstWebRTCSessionDescription* offer = createLocalOffer(handles.webrtcbin);
    ASSERT_NE(offer, nullptr) << "create-offer returned no offer";

    const std::string offerText = toText(offer->sdp);
    ASSERT_EQ(gst_sdp_message_medias_len(offer->sdp), 1u) << offerText;

    const GstSDPMedia* firstMedia = gst_sdp_message_get_media(offer->sdp, 0);
    ASSERT_NE(firstMedia, nullptr);
    const std::string firstMediaText = toText(firstMedia);
    const gchar* firstMid = gst_sdp_media_get_attribute_val(firstMedia, "mid");

    EXPECT_STREQ(gst_sdp_media_get_media(firstMedia), "video") << offerText;
    ASSERT_NE(firstMid, nullptr) << offerText;
    EXPECT_TRUE(startsWith(firstMid, "video")) << offerText;
    EXPECT_NE(firstMediaText.find("a=sendonly"), std::string::npos) << offerText;
    EXPECT_EQ(offerText.find("65535"), std::string::npos) << offerText;
    EXPECT_NE(
        offerText.find("m=video 9 UDP/TLS/RTP/SAVPF " + std::to_string(kH264PayloadType)),
        std::string::npos
    ) << offerText;
    EXPECT_NE(
        offerText.find("a=rtpmap:" + std::to_string(kH264PayloadType) + " H264/90000"),
        std::string::npos
    ) << offerText;
    EXPECT_NE(offerText.find("a=fmtp:" + std::to_string(kH264PayloadType)), std::string::npos)
        << offerText;
    EXPECT_NE(offerText.find("a=rtcp-fb:" + std::to_string(kH264PayloadType)), std::string::npos)
        << offerText;

    gst_webrtc_session_description_free(offer);
    releaseRequestedPad(handles.webrtcbin, activeSinkPad);
    destroyPipelineHandles(handles);
}

TEST(PipelineBuilder, CreatesServerOfferWithSingleVideoSlotAndDataChannel) {
    ensureGStreamerInitialized();

    WebRtcStreamingConfig cfg;
    PipelineHandles handles = buildPipeline(cfg, ImageSize{1280, 720});

    ASSERT_NE(handles.pipeline, nullptr);
    ASSERT_NE(handles.webrtcbin, nullptr);
    ASSERT_NE(handles.videoAppSrc, nullptr);
    ASSERT_NE(handles.videoPayloader, nullptr);

    const GstStateChangeReturn stateChange =
        gst_element_set_state(handles.pipeline, GST_STATE_READY);
    ASSERT_NE(stateChange, GST_STATE_CHANGE_FAILURE);

    GstPad* activeSinkPad = requestSinkPad(handles.webrtcbin, kLegacyRgbVideoMLineIndex);
    ASSERT_NE(activeSinkPad, nullptr) << "failed to request sink_0";
    ASSERT_TRUE(configurePadTransceiverCodecPreferences(activeSinkPad))
        << "failed to set H264 codec preferences on sink_0";
    ASSERT_TRUE(configurePadTransceiverDirection(
        activeSinkPad, GST_WEBRTC_RTP_TRANSCEIVER_DIRECTION_SENDONLY
    ));
    expectPadTransceiverDirection(activeSinkPad, GST_WEBRTC_RTP_TRANSCEIVER_DIRECTION_SENDONLY);

    ASSERT_TRUE(linkPayloaderToSinkPad(handles.videoPayloader, activeSinkPad))
        << "failed to link pay_video to sink_0";

    GstWebRTCDataChannel* dataChannel = createLocalDataChannel(handles.webrtcbin);
    ASSERT_NE(dataChannel, nullptr) << "failed to create local data channel";
    logPadTransceiverState("single-video+data sink_0", activeSinkPad);
    logKnownTransceivers(handles.webrtcbin, "single-video+data before create-offer");

    GstWebRTCSessionDescription* offer = createLocalOffer(handles.webrtcbin);
    ASSERT_NE(offer, nullptr) << "create-offer returned no offer";

    const std::string offerText = toText(offer->sdp);
    ASSERT_EQ(gst_sdp_message_medias_len(offer->sdp), 2u) << offerText;

    const GstSDPMedia* firstMedia = gst_sdp_message_get_media(offer->sdp, 0);
    const GstSDPMedia* secondMedia = gst_sdp_message_get_media(offer->sdp, 1);
    ASSERT_NE(firstMedia, nullptr);
    ASSERT_NE(secondMedia, nullptr);

    const std::string firstMediaText = toText(firstMedia);
    const std::string secondMediaText = toText(secondMedia);
    const gchar* firstMid = gst_sdp_media_get_attribute_val(firstMedia, "mid");
    const gchar* secondMid = gst_sdp_media_get_attribute_val(secondMedia, "mid");
    const gchar* bundleGroup = gst_sdp_message_get_attribute_val(offer->sdp, "group");

    EXPECT_STREQ(gst_sdp_media_get_media(firstMedia), "video") << offerText;
    ASSERT_NE(firstMid, nullptr) << offerText;
    EXPECT_TRUE(startsWith(firstMid, "video")) << offerText;
    EXPECT_NE(firstMediaText.find("a=sendonly"), std::string::npos) << offerText;
    EXPECT_EQ(offerText.find("65535"), std::string::npos) << offerText;
    EXPECT_NE(
        offerText.find("m=video 9 UDP/TLS/RTP/SAVPF " + std::to_string(kH264PayloadType)),
        std::string::npos
    ) << offerText;
    EXPECT_NE(
        offerText.find("a=rtpmap:" + std::to_string(kH264PayloadType) + " H264/90000"),
        std::string::npos
    ) << offerText;
    EXPECT_NE(offerText.find("a=fmtp:" + std::to_string(kH264PayloadType)), std::string::npos)
        << offerText;
    EXPECT_NE(offerText.find("a=rtcp-fb:" + std::to_string(kH264PayloadType)), std::string::npos)
        << offerText;

    EXPECT_STREQ(gst_sdp_media_get_media(secondMedia), "application") << offerText;
    ASSERT_NE(secondMid, nullptr) << offerText;
    EXPECT_TRUE(startsWith(secondMid, "application")) << offerText;
    EXPECT_NE(secondMediaText.find("a=sctp-port:"), std::string::npos) << offerText;

    ASSERT_NE(bundleGroup, nullptr) << offerText;
    EXPECT_EQ(std::string{bundleGroup}, "BUNDLE " + std::string{firstMid} + " " + secondMid)
        << offerText;

    gst_webrtc_session_description_free(offer);
    gst_webrtc_data_channel_close(dataChannel);
    gst_object_unref(dataChannel);
    releaseRequestedPad(handles.webrtcbin, activeSinkPad);
    destroyPipelineHandles(handles);
}

TEST(PipelineBuilder, CreatesServerOfferWithTwoVideoSlots) {
    ensureGStreamerInitialized();

    WebRtcStreamingConfig cfg;
    PipelineHandles handles = buildPipeline(cfg, ImageSize{1280, 720});

    ASSERT_NE(handles.pipeline, nullptr);
    ASSERT_NE(handles.webrtcbin, nullptr);
    ASSERT_NE(handles.videoAppSrc, nullptr);
    ASSERT_NE(handles.videoPayloader, nullptr);

    const GstStateChangeReturn stateChange =
        gst_element_set_state(handles.pipeline, GST_STATE_READY);
    ASSERT_NE(stateChange, GST_STATE_CHANGE_FAILURE);

    GstPad* inactiveSinkPad = requestSinkPad(handles.webrtcbin, kLegacyRgbVideoMLineIndex);
    ASSERT_NE(inactiveSinkPad, nullptr) << "failed to request sink_0";
    GstPad* activeSinkPad = requestSinkPad(handles.webrtcbin, kLegacyTsdfVideoMLineIndex);
    ASSERT_NE(activeSinkPad, nullptr) << "failed to request sink_1";

    ASSERT_TRUE(configurePadTransceiverCodecPreferences(inactiveSinkPad));
    ASSERT_TRUE(configurePadTransceiverCodecPreferences(activeSinkPad));
    ASSERT_TRUE(configurePadTransceiverDirection(
        inactiveSinkPad, GST_WEBRTC_RTP_TRANSCEIVER_DIRECTION_INACTIVE
    ));
    ASSERT_TRUE(configurePadTransceiverDirection(
        activeSinkPad, GST_WEBRTC_RTP_TRANSCEIVER_DIRECTION_SENDONLY
    ));
    expectPadTransceiverDirection(inactiveSinkPad, GST_WEBRTC_RTP_TRANSCEIVER_DIRECTION_INACTIVE);
    expectPadTransceiverDirection(activeSinkPad, GST_WEBRTC_RTP_TRANSCEIVER_DIRECTION_SENDONLY);

    ASSERT_TRUE(linkPayloaderToSinkPad(handles.videoPayloader, activeSinkPad))
        << "failed to link pay_video to sink_1";
    logPadTransceiverState("two-video sink_0", inactiveSinkPad);
    logPadTransceiverState("two-video sink_1", activeSinkPad);
    logKnownTransceivers(handles.webrtcbin, "two-video before create-offer");

    GstWebRTCSessionDescription* offer = createLocalOffer(handles.webrtcbin);
    ASSERT_NE(offer, nullptr) << "create-offer returned no offer";

    const std::string offerText = toText(offer->sdp);
    ASSERT_EQ(gst_sdp_message_medias_len(offer->sdp), 2u) << offerText;

    const GstSDPMedia* firstMedia = gst_sdp_message_get_media(offer->sdp, 0);
    const GstSDPMedia* secondMedia = gst_sdp_message_get_media(offer->sdp, 1);
    ASSERT_NE(firstMedia, nullptr);
    ASSERT_NE(secondMedia, nullptr);

    const std::string firstMediaText = toText(firstMedia);
    const std::string secondMediaText = toText(secondMedia);
    const gchar* firstMid = gst_sdp_media_get_attribute_val(firstMedia, "mid");
    const gchar* secondMid = gst_sdp_media_get_attribute_val(secondMedia, "mid");

    EXPECT_STREQ(gst_sdp_media_get_media(firstMedia), "video") << offerText;
    ASSERT_NE(firstMid, nullptr) << offerText;
    EXPECT_TRUE(startsWith(firstMid, "video")) << offerText;
    EXPECT_NE(firstMediaText.find("a=inactive"), std::string::npos) << offerText;

    EXPECT_STREQ(gst_sdp_media_get_media(secondMedia), "video") << offerText;
    ASSERT_NE(secondMid, nullptr) << offerText;
    EXPECT_TRUE(startsWith(secondMid, "video")) << offerText;
    EXPECT_STRNE(firstMid, secondMid) << offerText;
    EXPECT_NE(secondMediaText.find("a=sendonly"), std::string::npos) << offerText;

    gst_webrtc_session_description_free(offer);
    releaseRequestedPad(handles.webrtcbin, activeSinkPad);
    releaseRequestedPad(handles.webrtcbin, inactiveSinkPad);
    destroyPipelineHandles(handles);
}

TEST(PipelineBuilder, CreatesServerOfferWithTwoVideoSlotsAndDataChannel) {
    ensureGStreamerInitialized();

    WebRtcStreamingConfig cfg;
    PipelineHandles handles = buildPipeline(cfg, ImageSize{1280, 720});

    ASSERT_NE(handles.pipeline, nullptr);
    ASSERT_NE(handles.webrtcbin, nullptr);
    ASSERT_NE(handles.videoAppSrc, nullptr);
    ASSERT_NE(handles.videoPayloader, nullptr);

    const GstStateChangeReturn stateChange =
        gst_element_set_state(handles.pipeline, GST_STATE_READY);
    ASSERT_NE(stateChange, GST_STATE_CHANGE_FAILURE);

    GstPad* inactiveSinkPad = requestSinkPad(handles.webrtcbin, kLegacyRgbVideoMLineIndex);
    ASSERT_NE(inactiveSinkPad, nullptr) << "failed to request sink_0";
    GstPad* activeSinkPad = requestSinkPad(handles.webrtcbin, kLegacyTsdfVideoMLineIndex);
    ASSERT_NE(activeSinkPad, nullptr) << "failed to request sink_1";

    ASSERT_TRUE(configurePadTransceiverCodecPreferences(inactiveSinkPad));
    ASSERT_TRUE(configurePadTransceiverCodecPreferences(activeSinkPad));
    ASSERT_TRUE(configurePadTransceiverDirection(
        inactiveSinkPad, GST_WEBRTC_RTP_TRANSCEIVER_DIRECTION_INACTIVE
    ));
    ASSERT_TRUE(configurePadTransceiverDirection(
        activeSinkPad, GST_WEBRTC_RTP_TRANSCEIVER_DIRECTION_SENDONLY
    ));
    expectPadTransceiverDirection(inactiveSinkPad, GST_WEBRTC_RTP_TRANSCEIVER_DIRECTION_INACTIVE);
    expectPadTransceiverDirection(activeSinkPad, GST_WEBRTC_RTP_TRANSCEIVER_DIRECTION_SENDONLY);

    ASSERT_TRUE(linkPayloaderToSinkPad(handles.videoPayloader, activeSinkPad))
        << "failed to link pay_video to sink_1";

    GstWebRTCDataChannel* dataChannel = createLocalDataChannel(handles.webrtcbin);
    ASSERT_NE(dataChannel, nullptr) << "failed to create local data channel";
    logPadTransceiverState("two-video+data sink_0", inactiveSinkPad);
    logPadTransceiverState("two-video+data sink_1", activeSinkPad);
    logKnownTransceivers(handles.webrtcbin, "two-video+data before create-offer");

    GstWebRTCSessionDescription* offer = createLocalOffer(handles.webrtcbin);
    ASSERT_NE(offer, nullptr) << "create-offer returned no offer";

    const std::string offerText = toText(offer->sdp);
    ASSERT_EQ(gst_sdp_message_medias_len(offer->sdp), 3u) << offerText;
    const gchar* bundleGroup = gst_sdp_message_get_attribute_val(offer->sdp, "group");

    const GstSDPMedia* firstMedia = gst_sdp_message_get_media(offer->sdp, 0);
    const GstSDPMedia* secondMedia = gst_sdp_message_get_media(offer->sdp, 1);
    const GstSDPMedia* thirdMedia = gst_sdp_message_get_media(offer->sdp, 2);
    ASSERT_NE(firstMedia, nullptr);
    ASSERT_NE(secondMedia, nullptr);
    ASSERT_NE(thirdMedia, nullptr);

    const std::string firstMediaText = toText(firstMedia);
    const std::string secondMediaText = toText(secondMedia);
    const std::string thirdMediaText = toText(thirdMedia);
    const gchar* firstMid = gst_sdp_media_get_attribute_val(firstMedia, "mid");
    const gchar* secondMid = gst_sdp_media_get_attribute_val(secondMedia, "mid");
    const gchar* thirdMid = gst_sdp_media_get_attribute_val(thirdMedia, "mid");

    EXPECT_STREQ(gst_sdp_media_get_media(firstMedia), "video") << offerText;
    ASSERT_NE(firstMid, nullptr) << offerText;
    EXPECT_TRUE(startsWith(firstMid, "video")) << offerText;
    EXPECT_NE(firstMediaText.find("a=inactive"), std::string::npos) << offerText;

    EXPECT_STREQ(gst_sdp_media_get_media(secondMedia), "video") << offerText;
    ASSERT_NE(secondMid, nullptr) << offerText;
    EXPECT_TRUE(startsWith(secondMid, "video")) << offerText;
    EXPECT_STRNE(firstMid, secondMid) << offerText;
    EXPECT_NE(secondMediaText.find("a=sendonly"), std::string::npos) << offerText;

    EXPECT_STREQ(gst_sdp_media_get_media(thirdMedia), "application") << offerText;
    ASSERT_NE(thirdMid, nullptr) << offerText;
    EXPECT_TRUE(startsWith(thirdMid, "application")) << offerText;
    EXPECT_NE(thirdMediaText.find("a=sctp-port:"), std::string::npos) << offerText;
    ASSERT_NE(bundleGroup, nullptr) << offerText;
    EXPECT_EQ(
        std::string{bundleGroup},
        "BUNDLE " + std::string{firstMid} + " " + secondMid + " " + thirdMid
    ) << offerText;

    gst_webrtc_session_description_free(offer);
    gst_webrtc_data_channel_close(dataChannel);
    gst_object_unref(dataChannel);
    releaseRequestedPad(handles.webrtcbin, activeSinkPad);
    releaseRequestedPad(handles.webrtcbin, inactiveSinkPad);

    destroyPipelineHandles(handles);
}
