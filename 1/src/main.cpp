#include <cassert>
#include <iostream>
#include <string>
#include <utility>

#include "shared_ptr.hpp"

namespace {

int g_alive = 0;

struct Base {
    Base() { ++g_alive; }
    virtual ~Base() { --g_alive; }
    virtual std::string name() const { return "Base"; }
};

struct Derived : Base {
    std::string name() const override { return "Derived"; }
};

}  // namespace

int main() {
    {
        SharedPtr<Base> a(new Derived);
        assert(a.use_count() == 1 && a->name() == "Derived" && (*a).name() == "Derived");

        SharedPtr<Base> b = a;
        assert(a.use_count() == 2 && a == b);

        SharedPtr<Base> c = std::move(b);
        assert(!b && b == nullptr && c.use_count() == 2);

        SharedPtr<Derived> d(new Derived);
        SharedPtr<Base> e = d;
        assert(e.use_count() == 2 && g_alive == 2);

        swap(a, e);
        assert(a.get() == d.get());
        assert((a < c) != (c < a) && a != c);

        c.reset();
        e.reset();
        assert(g_alive == 1);

        a = nullptr;
        a.reset(new Base);
        assert(g_alive == 2);
    }
    assert(g_alive == 0);

    std::cout << "SharedPtr: all checks passed\n";
    return 0;
}
