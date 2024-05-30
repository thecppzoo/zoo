#include "zoo/Any/DerivedVTablePolicy.h"
#include "zoo/AnyContainer.h"

#include <math.h>
#include <stdint.h>

/* Some definitions necessary, they are not in the listings */
using f32 = float;
constexpr f32 Pi32 = M_PI;
using u32 = uint32_t;

/* ========================================================================
   LISTING 22
   ======================================================================== */

class shape_base
{
public:
    shape_base() {}
    virtual f32 Area() = 0;
};

class square : public shape_base
{
public:
    square(f32 SideInit) : Side(SideInit) {}
    f32 AreaNP() {return Side*Side;}
    virtual f32 Area() {return Side*Side;}
    
private:
    f32 Side;
};

class rectangle : public shape_base
{
public:
    rectangle(f32 WidthInit, f32 HeightInit) : Width(WidthInit), Height(HeightInit) {}
    f32 AreaNP() {return Width*Height;}
    virtual f32 Area() {return Width*Height;}
    
private:
    f32 Width, Height;
};

class triangle : public shape_base
{
public:
    triangle(f32 BaseInit, f32 HeightInit) : Base(BaseInit), Height(HeightInit) {}
    f32 AreaNP() {return 0.5f*Base*Height;}
    virtual f32 Area() {return 0.5f*Base*Height;}
    
private:
    f32 Base, Height;
};

class circle : public shape_base
{
public:
    circle(f32 RadiusInit) : Radius(RadiusInit) {}
    f32 AreaNP() {return Pi32*Radius*Radius;}
    virtual f32 Area() {return Pi32*Radius*Radius;}
    
private:
    f32 Radius;
};

/* ========================================================================
   LISTING 23
   ======================================================================== */

f32 TotalAreaVTBL(u32 ShapeCount, shape_base **Shapes)
{
    f32 Accum = 0.0f;
    for(u32 ShapeIndex = 0; ShapeIndex < ShapeCount; ++ShapeIndex)
    {
        Accum += Shapes[ShapeIndex]->Area();
    }
    
    return Accum;
}

#include <chrono>
#include <memory>
#include <vector>
#include <random>
#include <stdexcept>
#include <atomic>

std::atomic_int sideEffect = 0;

template<typename F, typename... Args>
auto benchmark(F &&f, Args &&...args) {
    auto start = std::chrono::high_resolution_clock::now();
    sideEffect = f(std::forward<Args>(args)...);
    auto end = std::chrono::high_resolution_clock::now();
    return end - start;
}

template <typename Duration>
auto toNanoseconds(Duration &&duration) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
}

template<typename G, typename D>
shape_base *buildShape(int shapeTypeIndex, G &&g, D &&d) {
    switch(shapeTypeIndex) {
        case 0: return new square(d(g));
        case 1: return new rectangle(d(g), d(g));
        case 2: return new triangle(d(g), d(g));
        case 3: return new circle(d(g));
        default: throw std::runtime_error("Impossible shape allocation");
    }
}

struct ShapeCollection {
    std::vector<shape_base *> storage_;
    static inline int deletions_ = 0;

    template<typename G>
    ShapeCollection(G &&g, int count) {
        std::uniform_int_distribution shape(0, 2);
        std::uniform_real_distribution rd(0.00001, 1.2);
        for(auto c = count; c--; ) {
            std::unique_ptr<shape_base> temporary(buildShape(shape(g), g, rd));
            storage_.push_back(temporary.get());
            temporary.release();
        }
    }

    ~ShapeCollection() {
        for(auto ptr: storage_) {
            delete ptr;
            ++deletions_;
        }
    }
};


#include <iostream>

struct ShapeAffordance {
    template<typename> struct Mixin {};

    struct VTableEntry { f32 (*area)(void *); };

    template<typename>
    constexpr static VTableEntry DefaultImplementation =
        { [](void *) { return 0.0f; } };
    template<typename VM>
    constexpr static VTableEntry Operation = {
        [](void *p) {
            auto vm = static_cast<VM *>(p);
            auto value = reinterpret_cast<typename VM::ManagedType *>(vm->value());
            return value->AreaNP();
        }
    };

    template<typename AC>
    struct UserAffordance {
        f32 area() {
            AC *ac = static_cast<AC *>(this);
            auto vm = ac->container();
            return vm->template vTable<ShapeAffordance>()->area(vm);
        }
    };
};

using P = zoo::Policy<void *[2], ShapeAffordance, zoo::Destroy, zoo::Move>;
using Shape = zoo::AnyContainer<P>;

static_assert(24 == sizeof(Shape));

auto zooTraverse(std::vector<Shape> &s) {
    f32 rv = 0;
    for(auto &shape: s) {
        rv += shape.area();
    }
    return rv;
}

int main(int argc, const char *argv[]) {
    std::random_device rd;
    std::mt19937 mersenne(rd());
    constexpr auto Count = 1'048'583;
    //constexpr auto Count = 32749;
    auto virtualInheritance = [](auto &&mersenne) {
        ShapeCollection collection(mersenne, Count);
        auto dur = benchmark(
            [](auto c, auto ptrs) { return static_cast<int>(TotalAreaVTBL(c, ptrs)); },
            collection.storage_.size(),
            collection.storage_.data()
        );
        return dur;
    };
    auto zooTypeErasure = [](auto &&m) {
        std::vector<Shape> shapes;
        std::uniform_int_distribution sti(0, 2);
        std::uniform_real_distribution rd(0.00001, 1.2);
        for(int i = Count; i--; ) {
            auto shapeTypeIndex = sti(m);
            switch(shapeTypeIndex) {
                case 0: shapes.push_back(square(rd(m))); break;
                case 1: shapes.push_back(rectangle(rd(m), rd(m))); break;
                case 2: shapes.push_back(triangle(rd(m), rd(m))); break;
                case 3: shapes.push_back(circle(rd(m))); break;
                default: throw std::runtime_error("Impossible shape type index");
            }
        }
        return
            benchmark(
                zooTraverse,
                shapes
            );
    };
    auto report = [](auto duration) {
        auto nanos = toNanoseconds(duration);
        constexpr auto Cycle = 1/3.6;
        std::cout << nanos << ' ' << nanos/double(Count) << ' ' << nanos/Cycle/Count << ' ' << sideEffect << std::endl;
    };
    auto zte1 = zooTypeErasure(mersenne);
    auto durationVI = virtualInheritance(mersenne);
    auto zte2 = zooTypeErasure(mersenne);
    report(zte1);
    report(durationVI);
    report(zte2);
    return 0;
}
