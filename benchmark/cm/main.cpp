#include "zoo/Any/DerivedVTablePolicy.h"
#include "zoo/AnyContainer.h"

#include <math.h>
#include <stdint.h>

#include "variant.h"

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
struct VariantShape: zoo::Variant<square, rectangle, triangle, circle> {
    using Base = zoo::Variant<square, rectangle, triangle, circle>;
    using Base::Base;

    f32 area() {
        auto rv = zoo::visit<f32>(
            [](auto &s) {
                if constexpr (std::is_same_v<zoo::BadVariant &, decltype(s)>) {
                    return 0.0;
                } else {
                    return s.AreaNP();
                }
            },
            *this
        );
        return rv;
    }
};

template<typename E>
auto zooTraverse(std::vector<E> &s) {
    f32 rv = 0;
    for(auto &shape: s) {
        rv += shape.area();
    }
    return rv;
}

/* ========================================================================
   LISTING 25
   ======================================================================== */

enum shape_type : u32
{
    Shape_Square,
    Shape_Rectangle,
    Shape_Triangle,
    Shape_Circle,
    
    Shape_Count,
};

struct shape_union
{
    shape_type Type;
    f32 Width;
    f32 Height;
};

f32 GetAreaSwitch(shape_union Shape)
{
    f32 Result = 0.0f;
    
    switch(Shape.Type)
    {
        case Shape_Square: {Result = Shape.Width*Shape.Width;} break;
        case Shape_Rectangle: {Result = Shape.Width*Shape.Height;} break;
        case Shape_Triangle: {Result = 0.5f*Shape.Width*Shape.Height;} break;
        case Shape_Circle: {Result = Pi32*Shape.Width*Shape.Width;} break;
        
        case Shape_Count: {} break;
    }
    
    return Result;
}

/* ========================================================================
   LISTING 26
   ======================================================================== */

f32 TotalAreaSwitch(u32 ShapeCount, shape_union *Shapes)
{
    f32 Accum = 0.0f;
    
    for(u32 ShapeIndex = 0; ShapeIndex < ShapeCount; ++ShapeIndex)
    {
        Accum += GetAreaSwitch(Shapes[ShapeIndex]);
    }

    return Accum;
}

f32 TotalAreaSwitch4(u32 ShapeCount, shape_union *Shapes)
{
    f32 Accum0 = 0.0f;
    f32 Accum1 = 0.0f;
    f32 Accum2 = 0.0f;
    f32 Accum3 = 0.0f;
    
    ShapeCount /= 4;
    while(ShapeCount--)
    {
        Accum0 += GetAreaSwitch(Shapes[0]);
        Accum1 += GetAreaSwitch(Shapes[1]);
        Accum2 += GetAreaSwitch(Shapes[2]);
        Accum3 += GetAreaSwitch(Shapes[3]);
        
        Shapes += 4;
    }
    
    f32 Result = (Accum0 + Accum1 + Accum2 + Accum3);
    return Result;
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
                zooTraverse<Shape>,
                shapes
            );
    };
    auto zooVariant = [](auto &&m) {
        std::vector<VariantShape> shapes;
        std::uniform_int_distribution sti(0, 2);
        std::uniform_real_distribution<float> rd(0.00001, 1.2);
        for(int i = Count; i--; ) {
            auto shapeTypeIndex = sti(m);
            switch(shapeTypeIndex) {
                case 0: shapes.emplace_back(std::in_place_index<0>, square(rd(m))); break;
                case 1: shapes.emplace_back(std::in_place_index<1>, rectangle(rd(m), rd(m))); break;
                case 2: shapes.emplace_back(std::in_place_index<2>, triangle(rd(m), rd(m))); break;
                case 3: shapes.emplace_back(std::in_place_index<3>, circle(rd(m))); break;
                default: throw std::runtime_error("Impossible shape type index");
            }
        }
        return
            benchmark(
                zooTraverse<VariantShape>,
                shapes
            );
    };
    auto CaseyMuratoriUnion = [](auto &&m) {
        std::vector<shape_union> shapes;
        std::uniform_int_distribution sti(0, 2);
        std::uniform_real_distribution<float> rd(0.00001, 1.2);
        for(int i = Count; i--; ) {
            auto shapeTypeIndex = sti(m);
            shape_union element = { static_cast<shape_type>(shapeTypeIndex), rd(m), rd(m) };
            shapes.push_back(element);
        }
        return
            benchmark(
                TotalAreaSwitch,
                shapes.size(),
                shapes.data()
            );
    };
    auto report = [](auto duration, auto txt) {
        auto nanos = toNanoseconds(duration);
        constexpr auto Cycle = 1/3.6;
        std::cout << nanos << ' ' << nanos/double(Count) << ' ' << nanos/Cycle/Count << ' ' << sideEffect << ' ' << txt << std::endl;
    };
    auto zte1 = zooTypeErasure(mersenne);
    auto durationVI = virtualInheritance(mersenne);
    auto zooVar = zooVariant(mersenne);
    auto cmSwitch = CaseyMuratoriUnion(mersenne);
    auto zte2 = zooTypeErasure(mersenne);
    report(zte1, "ZTE 1");
    report(durationVI, "Virtual + inheritance");
    report(zooVar, "Zoo variant");
    report(cmSwitch, "Casey Muratori's SWITCH");
    report(zte2, "ZTE 2");
    return 0;
}
