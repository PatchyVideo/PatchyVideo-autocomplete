
#include <memory>
#include <string>

struct Keyword
{
    std::uint32_t id;
    std::string str;
};

int main()
{
    std::unique_ptr<Keyword> tag(std::make_unique<Keyword>(123, "a41234"));
}
