# Diagrams of operations in the Virtual Table Policy and Affordance Framework.

## Note: Some of the identifiers in the framework come from very old times, when the concepts where more primitive.

## The base value manager

The *policy* tells `AnyContainer` what its local storage is going to be, via the member `MemoryLayout`.  You may think of the "Memory Layout" as the base ***value manager***.

The ownership operations (destruction, moving, copying) require direct support from the framework, that's why they must be provided by the "value managers", via functions like `copyOp`, `move`, `destructor`.  Everything else is provided directly by the ***affordances***.  The value managers implement `copyOp`, `move`, `destructor` deferring to the affordances, this allows the user to choose any mechanism.  For example, you can implement and select an affordance of destruction that performs *tracing* of the destruction of some opt-in objects.

The most important feature of this flavor of the framework is to use virtual tables as the *type switching*.  The base value manager inherits from a "virtual table holder" class that controls access to the virtual table:

1. The `VTHolder` is a base class to guarantee the virtual table pointer is the first member in the memory layout of the base value manager, for performance.
2. The type of the virtual table is the aggregation of the `VTableEntry` types of each affordance: [`struct VTable: AffordanceSpecifications::VTableEntry... {`](https://github.com/thecppzoo/zoo/blob/d6435fc984ee0bde31979f7908a73473f61ac4bd/inc/zoo/Any/VTablePolicy.h#L247C5-L247C60)
3. The `VTHolder` allows every affordance to access its part of the virtual table via the template member `vTable`, if `Affordance` is the type representing an affordance, `baseManager->template vTable<Affordance>()` returns that section of the virtual table.
4. The base value manager is the provider of the implementations for the default constructed `AnyContainer`.
    1. Implementations are either `constexpr` data, or more typically, the pointers to the functions that implement the behaviors.  The base manager must defer to the affordances for these values, in particular to the `Default` class-members of the affordances, which are assumed to be of their `VTableEntry` type [^DefaultImplementations]
6. The so-called "concrete value managers" provide the implementations for the value they manage.

[^DefaultImplementations]:
```c++
        constexpr static inline VTable Default = {
            AffordanceSpecifications::template Default<Container>...
        };
```
[from here](https://github.com/thecppzoo/zoo/blob/d6435fc984ee0bde31979f7908a73473f61ac4bd/inc/zoo/Any/VTablePolicy.h#L274-L276).

