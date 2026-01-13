#include <iostream>
#include <vector>
#include <string>
#include <tuple>
#include <stdexcept>
#include <optional>
#include <memory>
#include <map>
#include <typeindex>

// This is the core Operation structure that holds the state of an operation.
// Currently, it only has a name and a list of integer operands for simplicity.
struct Operation {
  std::string name;
  std::vector<int> operands;
};

// This is the Op Base class template that all concrete operation views
// will inherit from. It is based on the CRTP pattern, as indicated by the
// Derived template parameter. The class also takes a variadic list of Traits
// that represent the interfaces the operation supports.
template <typename Derived, typename... Traits> class Op {
protected:
  Operation* state;

public:
  explicit Op(Operation* state) : state(state) {}

  Operation* getOperation() const { return state; }

  static std::string getName() { return Derived::name; }

  // Use fold expression and type-traits to check if a trait is supported.
  template <typename T>
  static constexpr bool has_trait = (std::is_same_v<T, Traits> || ...);
};

// InterfaceRegistry class that maintains a global registry of interfaces
// associated with operation names. It allows registering and looking up
// interface implementations for specific operations.
class InterfaceRegistry {
public:
  // The nested map structure is the specific mechanism that allows
  // Many-to-Many relationships between Operations and Interfaces at runtime.
  using InterfaceMap = std::map<std::type_index, void*>;
  using RegistryMap = std::map<std::string, InterfaceMap>;

  static RegistryMap& get() {
    static RegistryMap instance;
    return instance;
  }

  template <typename InterfaceTrait>
  static void registerInterface(const std::string& opName, void* model) {
    get()[opName][typeid(InterfaceTrait)] = model;
  }

  template <typename InterfaceTrait>
  static typename InterfaceTrait::Concept* lookup(const std::string& opName) {
    auto& reg = get();
    auto opIt = reg.find(opName);
    if (opIt == reg.end()) return nullptr;

    auto ifaceIt = opIt->second.find(typeid(InterfaceTrait));
    if (ifaceIt == opIt->second.end()) return nullptr;

    return static_cast<typename InterfaceTrait::Concept*>(ifaceIt->second);
  }
};

// BaseInterface class template that provides common functionality for all
// interfaces. It takes a DerivedInterface (CRTP) and a TraitsType that defines
// the interface's concept and model. It holds a pointer to the operation
// entity and a pointer to the virtual concept implementation.
template <typename DerivedInterface, typename TraitsType> class BaseInterface {
protected:
  void *entity;
  typename TraitsType::Concept *vconcept;

public:
  using Trait = TraitsType;

  BaseInterface(void *e, typename TraitsType::Concept *c)
      : entity(e), vconcept(c) {}

  typename TraitsType::Concept *getImpl() const { return vconcept; }
  void *getEntity() const { return entity; }
};

// SideEffectsInterfaceTraits definition
// It has a Concept struct that defines the interface for checking side effects.
// The Model struct template implements this interface for concrete operations.
// This is generally known as concept-based polymorphism.
// In normal polymorphism, we would have a base class with virtual methods.
// Here, we separate the interface (Concept) from the implementation (Model).
// This allows us to define interfaces that can be implemented by any type
// without requiring inheritance from a common base class.
struct SideEffectsInterfaceTraits {
  struct Concept {
    virtual ~Concept() = default;
    virtual bool hasSideEffect(void *opaqueOp) const = 0;
  };

  template <typename ConcreteOp> struct Model : public Concept {
    bool hasSideEffect(void *opaqueOp) const override {
      Operation* opState = static_cast<Operation*>(opaqueOp);
      ConcreteOp view(opState);
      return view.hasSideEffect();
    }
  };
};

// SideEffectsInterface definition
// This class inherits from BaseInterface using CRTP. It provides a method
// to check if the operation has side effects by delegating to the concept
// implementation.
class SideEffectsInterface
    : public BaseInterface<SideEffectsInterface, SideEffectsInterfaceTraits> {
public:
  using BaseInterface::BaseInterface;
  bool hasSideEffect() const { return getImpl()->hasSideEffect(getEntity()); }
};

// CostInterfaceTraits definition
// Similar to SideEffectsInterfaceTraits, this defines an interface for
// retrieving the cost of an operation. This has an ExternalModel struct
// template that allows defining the cost logic outside of the operation
// class itself.
struct CostInterfaceTraits {
  struct Concept {
    virtual ~Concept() = default;
    virtual int getCost(void *opaqueOp) const = 0;
  };

  template <typename ConcreteOp> struct Model : public Concept {
    int getCost(void *opaqueOp) const override {
      Operation* opState = static_cast<Operation*>(opaqueOp);
      ConcreteOp view(opState);
      return view.getCost();
    }
  };


  // ExternalModel allows defining the cost logic outside of the operation
  // class. This is useful for adding functionality to operations
  // without modifying their definitions. This template takes both
  // the ConcreteModel that implements the cost logic and the ConcreteOp
  // that represents the operation view.
  template <typename ConcreteModel, typename ConcreteOp>
  struct ExternalModel : public Concept {
    int getCost(void *opaqueOp) const override {
      Operation* opState = static_cast<Operation*>(opaqueOp);
      ConcreteOp view(opState);
      return static_cast<const ConcreteModel*>(this)->getCost(view);
    }
  };
};

class CostInterface : public BaseInterface<CostInterface, CostInterfaceTraits> {
public:
  using BaseInterface::BaseInterface;
  int getCost() const { return getImpl()->getCost(getEntity()); }
};

// Ataches an external interface model to a concrete operation.
// This function creates a static instance of the external model
// and registers it in the InterfaceRegistry for the given operation name.
template <typename ConcreteOp, typename InterfaceTrait, typename ExternalModelClass>
void attachInterface() {
  static ExternalModelClass model;
  InterfaceRegistry::registerInterface<InterfaceTrait>(ConcreteOp::name, &model);
}

class AddOp : public Op<AddOp, SideEffectsInterfaceTraits> {
public:
  using Op::Op;
  static constexpr const char* name = "addOp";

  int getLHS() const { return state->operands[0]; }
  int getRHS() const { return state->operands[1]; }
  bool hasSideEffect() { return false; }
};

class LoadOp : public Op<LoadOp, SideEffectsInterfaceTraits> {
public:
  using Op::Op;
  static constexpr const char* name = "loadOp";

  int getAddress() const { return state->operands[0]; }
  bool hasSideEffect() { return true; }
};

template <typename ConcreteOp, typename... Args>
std::unique_ptr<Operation> createOp(Args... args) {
  auto op = std::make_unique<Operation>();
  op->name = ConcreteOp::name;
  (op->operands.push_back(args), ...);
  return op;
}

template <typename Interface, typename Derived, typename... Traits>
std::optional<Interface> dyn_cast(Op<Derived, Traits...> op) {
  using RequiredTrait = typename Interface::Trait;

  if constexpr (Op<Derived, Traits...>::template has_trait<RequiredTrait>) {
    static typename RequiredTrait::template Model<Derived> static_model;
    return Interface(op.getOperation(), &static_model);
  }
  // if the operation does not have the trait statically, check the registry
  // for an external model.
  else {
    auto* external_model = InterfaceRegistry::lookup<RequiredTrait>(Derived::name);
    if (external_model) {
      return Interface(op.getOperation(), external_model);
    }
    return std::nullopt;
  }
}

template <typename ConcreteView>
std::optional<ConcreteView> dyn_cast_view(Operation* op) {
  if (op->name == ConcreteView::name) {
    return ConcreteView(op);
  }
  return std::nullopt;
}

struct AddOpCostModel
  : public CostInterfaceTraits::ExternalModel<AddOpCostModel, AddOp> {

  int getCost(AddOp op) const {
    return 1;
  }
};

struct LoadOpCostModel
  : public CostInterfaceTraits::ExternalModel<LoadOpCostModel, LoadOp> {

  int getCost(LoadOp op) const {
    return 50;
  }
};

int main() {
  std::vector<std::unique_ptr<Operation>> block;
  block.push_back(createOp<AddOp>(10, 20));
  block.push_back(createOp<LoadOp>(0xA000));

  std::cout << "=== 1. Before Registration (Should Fail) ===\n";

  for (const auto& ownedOp : block) {
    Operation* rawOp = ownedOp.get();

    if (auto addView = dyn_cast_view<AddOp>(rawOp)) {
       auto cost = dyn_cast<CostInterface>(*addView);
       if (!cost) std::cout << "AddOp does NOT have CostInterface yet.\n";
    }
  }

  std::cout << "\n[System] Registering External Models...\n";
  attachInterface<AddOp, CostInterfaceTraits, AddOpCostModel>();
  attachInterface<LoadOp, CostInterfaceTraits, LoadOpCostModel>();


  std::cout << "\n=== 2. After Registration (Should Succeed) ===\n";

  for (const auto& ownedOp : block) {
    Operation* rawOp = ownedOp.get();
    std::cout << "Op: " << rawOp->name;

    if (auto addView = dyn_cast_view<AddOp>(rawOp)) {
      if (auto costIface = dyn_cast<CostInterface>(*addView)) {
        std::cout << " -> Cost: " << costIface->getCost();
      }
    }
    else if (auto loadView = dyn_cast_view<LoadOp>(rawOp)) {
      if (auto costIface = dyn_cast<CostInterface>(*loadView)) {
        std::cout << " -> Cost: " << costIface->getCost();
      }
    }
    std::cout << "\n";
  }

  return 0;
}
