#include "zoo/Any/VTablePolicy.h"
#include "zoo/AnyContainer.h"

#include <memory> // for shared pointer

namespace user {

struct ExplicitDestructor {
    static inline void *last = nullptr;

    ~ExplicitDestructor() { last = this; }
};

template<typename>
struct SharedPointerOptIn: std::false_type {};

template<>
struct SharedPointerOptIn<int>: std::true_type {};

template<>
struct SharedPointerOptIn<ExplicitDestructor>: std::true_type {};

template<>
struct SharedPointerOptIn<std::string>: std::true_type {};


template<typename HoldingModel, typename... AffordanceSpecifications>
struct UserValueManagement {
    /// abbreviation
    using GP =
        typename zoo::GenericPolicy<HoldingModel, AffordanceSpecifications...>;

    template<typename V>
    struct SharedPointerManager:
        // Inherit for compatibility with the Generic Policy framework
        GP::Container
    {
        /// Abbreviation
        using Base = typename GP::Container;
        /// ManagedType is necessary for the affordances
        using ManagedType = V;
        /// Helper for this particular type of manager, VP for "Value Pointer"
        using VP = std::shared_ptr<V>;
        /// Abbreviation
        using SPM = SharedPointerManager;

        struct ExclusiveAware {};

        SharedPointerManager(): Base(&Operations) {
            new(sharedPointer()) VP;
        }

        // not part of the end-user interface
        VP *sharedPointer() noexcept { return this->space_.template as<VP>(); }
        // part of the end-user interface
        V *value() noexcept { return &**sharedPointer(); }

        const V *value() const noexcept {
            return const_cast<SPM *>(this)->value();
        }

        static void destructor(void *p) noexcept {
            auto sPtr = static_cast<SPM *>(p);
            sPtr->sharedPointer()->~VP();
        }

        static void move(void *to, void *from) noexcept {
            auto downcast = static_cast<SPM *>(from);
            new(to) SPM(std::move(*downcast));
        }

        static void copyOp(void *to, const void *from) {
            auto downcast = static_cast<SPM *>(const_cast<void *>(from));
            auto destValueManager = new(to) SPM;
            *destValueManager->sharedPointer() = *downcast->sharedPointer();
        }

        constexpr static inline typename GP::VTable Operations = {
            AffordanceSpecifications::template Operation<SPM>...
        };

        auto isExclusive() const noexcept {
            auto sp = const_cast<SPM *>(this)->sharedPointer();
            return 1 == sp->use_count();
        }

        auto makeExclusive() {
            *sharedPointer() = std::make_shared<V>(*value());
        }

        // not user interface
        SharedPointerManager(SharedPointerManager &&donor) noexcept:
            Base(&Operations)
        {
            new(sharedPointer()) VP(std::move(*donor.sharedPointer()));
        }

        // not user interface
//         SharedPointerManager(const SharedPointerManager &model) noexcept:
//             Base(&Operations)
//         {
//             new(sharedPointer()) VP(*const_cast<SPM &>(model).sharedPointer());
//         }



        // internal interface of Builder (important)
        template<typename... Args>
        SharedPointerManager(Args &&...args):
            Base(&Operations)
        {
            new(sharedPointer())
                VP(std::make_shared<V>(std::forward<Args>(args)...));
        }

        using IsReferenceTrait = std::false_type;
        constexpr static inline auto IsReference = IsReferenceTrait::value;
    };

    struct AdaptedPolicy: GP::Policy {
        // Builders is the old name to refer to what I now call "Value Manager"
        template<typename V>
        using Builder =
            std::conditional_t<
                SharedPointerOptIn<V>::value,
                SharedPointerManager<V>,
                typename GP::Policy::template Builder<V>
            >;
    };
};

template<typename H, typename... Afs>
using SharedPointerPolicy =
    typename UserValueManagement<H, Afs...>::AdaptedPolicy;

}
