#include <torch/torch.h>
#include <iostream>

int main() {
  auto a = torch::rand({2, 3});
  auto b = torch::rand({2, 3});
  auto c = a + b;
  std::cout << "torch ok: " << c.sum().item<double>() << "\n";
  return 0;
}
