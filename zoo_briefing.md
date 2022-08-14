zoo policy is like strategy design pattern, but follows alexandrescu

zoo policy has two components: memory layout, and a parameter pack of a set of affordances.

think is only setup that allows specifying layout and affordances with no cost beyond intrinsic.

Louis Dionne w/ boost::Dyno, Eric Niebler, at fb with folly::Poly

TypeErasureContainer has destroy/move affordances, which are minimal useful set.

There is an internal api that allows zoo function to interact with typeerasurecontainer/anycontainer, but there is no requirement that it be anycontainer/anycontainer in particular.

OrderConsumer is a container of generic container with a Signature which is implementation of the call operator() on closure pattern.

DerivedVTablePolicy allows copy and RTTI for no runtime cost, far beyond other available.

can rebind towards the root via reference rebinding.
void rebind(OrderedConsumder *oc, DV&& c) {
  *oc = std::move(c);
}

imake from int: results in putting the int value with the policy that contains layout and affordance.

initialize a func against a policy that contains layout and affordance. rdi this value of return value. 
  hoist call from vtable to instanct itself, resulting in layout like:
  memory layout: 
    hoisted function to invoke a contained object

template<typename>
int IntegerForType = 0;

template<> 
int IntegerForType<Signature *> = 1;

template struct IndexAffordance {
  template<typename> struct Mixin();
  // three capabilities
  // 1. calling 'index()' on it.
  // 3. accessing an integer constant value per-held-type
  // 2. accessing an integer value per-instance
  struct VTableEntry {
    int (*invokerOfIndex)(void*) noexcept;
    int thePerTypeInteger;
  }
  template <typename>
  constexpr static inline VTableEntry Default = {
    [](void*) noexcept { return -1; },
    -2
  }

  template <typename ConcreteValueManager> 
  constexpr static inline VTableEntry Operation  = {
    [](void*cvmPtr) noexcept {  // concrete value manager pointer
      using HeldType = typename ConcreteValueManager::ManagedType;
      if (constexpr (std::is_integral_v<HeldType>) {
        auto asCVMP = static_cast<ConcreteValueManager *>(cvmPtr);
        return __builtin_popcount(*cvmPtr->Value())
      } else return 0
IntegerForType<typename ConcreteValueManager::MergedType>

    }
    
  }
}


template<typename AnyC>
struct UserAffordance {
  int index() noexcept {
    AnyC *ac = static_cast<AnyC *>(this);  // CRTP downcast.
    auto valueManager = ac->container();
    auto vTable  = valueManager->template VTable<IndexAffordance>();
    return vTable->invokerOfIndex(valueManager);
  }

}

type erasure extending types allows us to improve our execution on affordances that we didn't even knwo we needed when we designed the system.



