//
// Created by johnk on 2022/8/10.
//

#pragma once

#include <tuple>

#include <entt/entt.hpp>
#include <taskflow/taskflow.hpp>

#include <Common/Utility.h>
#include <Common/Hash.h>
#include <Common/String.h>
#include <Common/Memory.h>
#include <Mirror/Meta.h>
#include <Mirror/Type.h>
#include <Runtime/Api.h>

#define DeclareSingleCompLifecycleEvent(eventClass) \
    struct EClass() eventClass : public Runtime::Event { \
        EClassBody(eventClass) \
        \
        EProperty() \
        Entity entity; \
    }; \

#define DeclareCompLifecycleEvents() \
    DeclareSingleCompLifecycleEvent(Added) \
    DeclareSingleCompLifecycleEvent(Updated) \
    DeclareSingleCompLifecycleEvent(Removed) \

#define EComponentBody(compClass) \
    EClassBody(compClass) \
    DeclareCompLifecycleEvents() \

#define DeclareSingleStateLifecycleEvent(eventClass) \
    struct EClass() eventClass : public Runtime::Event { \
        EClassBody(eventClass) \
    }; \

#define DeclareStateLifecycleEvents() \
    DeclareSingleStateLifecycleEvent(Added) \
    DeclareSingleStateLifecycleEvent(Updated) \
    DeclareSingleStateLifecycleEvent(Removed) \

#define EStateBody(stateClass) \
    EClassBody(stateClass) \
    DeclareStateLifecycleEvents() \

#define DeclareSystemDependencies(...) \
    using Dependencies = std::tuple<__VA_ARGS__>; \

namespace Runtime {
    enum class ClassSignatureType {
        staticClass,
        max
    };

    struct RUNTIME_API ClassSignature {
        ClassSignatureType type;
        size_t id;
        std::string name;

        bool operator==(const ClassSignature& rhs) const;
    };

    using ComponentSignature = ClassSignature;
    using StateSignature = ClassSignature;
    using SystemSignature = ClassSignature;
    using EventSignature = ClassSignature;
}

namespace std {
    template <>
    struct hash<Runtime::ClassSignature> {
        size_t operator()(const Runtime::ClassSignature& value) const
        {
            return std::hash<size_t>{}(value.id);
        }
    };
}

namespace Runtime::Internal {
    template <typename C>
    ClassSignature SignForStaticClass()
    {
        static ClassSignature signature = []() -> ClassSignature {
            const Mirror::Class& clazz = C::GetClass();

            std::tuple<ClassSignatureType, Mirror::TypeId> source = { ClassSignatureType::staticClass, clazz.GetTypeInfo()->id };

            ClassSignature result;
            result.type = ClassSignatureType::staticClass;
            result.id = clazz.GetTypeInfo()->id;
            result.name = clazz.GetName();
            return result;
        }();
        return signature;
    }

    template <typename S, typename C = void>
    struct SystemHasDependencies : std::false_type {};

    template <typename S>
    struct SystemHasDependencies<S, std::void_t<typename S::Dependencies>> : std::true_type {};

    template <typename Dependencies, size_t... I>
    std::vector<SystemSignature> BuildDependencyListForStaticSystemInternal(std::index_sequence<I...>)
    {
        std::vector<SystemSignature> result(std::tuple_size_v<Dependencies>);
        (void) std::initializer_list<int> { ([&]() -> void {
            result[I] = Internal::SignForStaticClass<std::tuple_element_t<I, Dependencies>>();
        }(), 0)... };
        return result;
    }

    template <typename S>
    std::vector<SystemSignature> BuildDependencyListForStaticSystem()
    {
        if constexpr (SystemHasDependencies<S>::value) {
            using Dependencies = typename S::Dependencies;
            return BuildDependencyListForStaticSystemInternal<Dependencies>(std::make_index_sequence<std::tuple_size_v<Dependencies>> {});
        } else {
            return std::vector<SystemSignature> {};
        }
    }
}

namespace Runtime {
    using Entity = entt::entity;
    class SystemCommands;
    static constexpr auto entityNull = entt::null;

    struct EClass() Component {
        EClassBody(Component)
    };

    struct EClass() State {
        EClassBody(State)
    };

    struct EClass() Event {
        EClassBody(Event)
    };

    struct EClass() System {
        EClassBody(System)
    };

    enum class SystemType : uint8_t {
        setup,
        tick,
        event,
        max
    };

    struct RUNTIME_API ECSHost {
    public:
        template <typename S>
        void AddSetupSystem()
        {
            SystemSignature signature = Internal::SignForStaticClass<S>();
            Assert(!systemInstances.contains(signature) && !setupSystems.contains(signature) && !setupSystemDependencies.contains(signature));

            auto* object = new S();
            SystemInstance instance;
            instance.type = SystemType::setup;
            instance.object = object;
            instance.setupFunc = [object](SystemCommands& commands) -> void {
                object->Setup(commands);
            };

            systemInstances.emplace(std::make_pair(signature, std::move(instance)));
            setupSystems.emplace(signature);
            setupSystemDependencies.emplace(std::make_pair(signature, Internal::BuildDependencyListForStaticSystem<S>()));
        }

        template <typename S>
        void AddTickSystem()
        {
            SystemSignature signature = Internal::SignForStaticClass<S>();
            Assert(!systemInstances.contains(signature) && !tickSystems.contains(signature) && !tickSystemDependencies.contains(signature));

            auto* object = new S();
            SystemInstance instance;
            instance.type = SystemType::tick;
            instance.object = object;
            instance.tickFunc = [object](SystemCommands& commands, float timeMS) -> void {
                object->Tick(commands, timeMS);
            };

            systemInstances.emplace(std::make_pair(signature, std::move(instance)));
            tickSystems.emplace(signature);
            tickSystemDependencies.emplace(std::make_pair(signature, Internal::BuildDependencyListForStaticSystem<S>()));
        }

        template <typename E, typename S>
        void AddEventSystem()
        {
            EventSignature eventSignature = Internal::SignForStaticClass<E>();
            if (!eventSystems.contains(eventSignature)) {
                eventSystems.emplace(std::make_pair(eventSignature, std::unordered_set<SystemSignature> {}));
                eventSystemDependencies.emplace(std::make_pair(eventSignature, std::unordered_map<SystemSignature, std::vector<SystemSignature>> {}));
            }

            auto& systems = eventSystems.at(eventSignature);
            auto& systemDependencies = eventSystemDependencies.at(eventSignature);
            SystemSignature systemSignature = Internal::SignForStaticClass<S>();
            Assert(!systemInstances.contains(systemSignature) && !systems.contains(systemSignature) && !systemDependencies.contains(systemSignature));

            auto* object = new S();
            SystemInstance instance;
            instance.type = SystemType::event;
            instance.object = object;
            instance.onReceiveFunc = [object](SystemCommands& commands, Mirror::Any* eventRef) -> void {
                object->OnReceive(commands, eventRef->As<const E&>());
            };

            systemInstances.emplace(std::make_pair(systemSignature, std::move(instance)));
            systems.emplace(systemSignature);
            systemDependencies.emplace(std::make_pair(systemSignature, Internal::BuildDependencyListForStaticSystem<S>()));
        }

        template <typename S>
        void RemoveSetupSystem()
        {
            SystemSignature signature = Internal::SignForStaticClass<S>();
            systemInstances.erase(signature);
            setupSystems.erase(signature);
            setupSystemDependencies.erase(signature);
        }

        template <typename S>
        void RemoveTickSystem()
        {
            SystemSignature signature = Internal::SignForStaticClass<S>();
            systemInstances.erase(signature);
            tickSystems.erase(signature);
            tickSystemDependencies.erase(signature);
        }

        template <typename E, typename S>
        void RemoveEventSystem()
        {
            EventSignature eventSignature = Internal::SignForStaticClass<E>();
            Assert(eventSystems.contains(eventSignature) && eventSystemDependencies.contains(eventSignature));

            auto& systems = eventSystems.at(eventSignature);
            auto& systemDependencies = eventSystemDependencies.at(eventSignature);

            SystemSignature systemSignature = Internal::SignForStaticClass<S>();
            systemInstances.erase(systemSignature);
            systems.erase(systemSignature);
            systemDependencies.erase(systemSignature);
        }

        template <typename E>
        void BroadcastEvent(const E& event)
        {
            EventSignature eventSignature = Internal::SignForStaticClass<E>();
            if (!eventSystems.contains(eventSignature)) {
                return;
            }

            const auto& systems = eventSystems.at(eventSignature);
            const auto& systemDependencies = eventSystemDependencies.at(eventSignature);

            tf::Taskflow taskflow;
            std::unordered_map<SystemSignature, tf::Task> tasks;
            tasks.reserve(systems.size());

            SystemCommands systemCommands(*this);
            Mirror::Any eventRef = std::ref(event);

            for (const auto& system : systems) {
                const auto& systemInstance = systemInstances.at(system);
                Assert(systemInstance.type == SystemType::event);

                tasks.emplace(std::make_pair(system, taskflow.emplace([&]() -> void {
                    systemInstance.onReceiveFunc(systemCommands, &eventRef);
                })));
            }

            for (const auto& dependencies : systemDependencies) {
                auto& task = tasks.at(dependencies.first);
                for (const auto& depend : dependencies.second) {
                    task.succeed(tasks.at(depend));
                }
            }
        }

    protected:
        virtual void Setup();
        virtual void Tick(float timeMS);
        virtual void Shutdown();

        ECSHost() = default;

        struct RUNTIME_API SystemInstance {
            SystemType type;
            Common::UniqueRef<System> object;
            union {
                std::function<void(SystemCommands&)> setupFunc;
                std::function<void(SystemCommands&, float timeMS)> tickFunc;
                std::function<void(SystemCommands&, Mirror::Any*)> onReceiveFunc;
            };

            SystemInstance();
            ~SystemInstance();
            SystemInstance(SystemInstance&& other) noexcept;
        };

        bool setuped;
        entt::registry registry;
        std::unordered_map<SystemSignature, SystemInstance> systemInstances;
        std::unordered_set<SystemSignature> setupSystems;
        std::unordered_set<SystemSignature> tickSystems;
        std::unordered_map<EventSignature, std::unordered_set<SystemSignature>> eventSystems;
        std::unordered_map<SystemSignature, std::vector<SystemSignature>> setupSystemDependencies;
        std::unordered_map<SystemSignature, std::vector<SystemSignature>> tickSystemDependencies;
        std::unordered_map<EventSignature, std::unordered_map<SystemSignature, std::vector<SystemSignature>>> eventSystemDependencies;
        std::unordered_map<StateSignature, Mirror::Any> states;

    private:
        friend class SystemCommands;
    };

    template <typename... Args>
    class Query {
    public:
        using Iterable = typename entt::view<Args...>::iterable_view;

        explicit Query(entt::view<Args...>&& inView)
            : view(std::move(inView))
        {
        }

        ~Query() = default;

        template <typename F>
        void Each(F&& func)
        {
            view.each(std::forward<F>(func));
        }

        auto Each()
        {
            return view.each();
        }

    private:
        entt::view<Args...> view;
    };

    template <typename... C>
    struct Exclude {};

    class SystemCommands;

    class RUNTIME_API SystemCommands {
    public:
        NonCopyable(SystemCommands)
        explicit SystemCommands(ECSHost& inHost);
        ~SystemCommands();

        Entity Create(Entity hint = entityNull);
        void Destroy(Entity inEntity);
        bool Valid(Entity inEntity) const;

        template <typename C, typename... Args>
        void Emplace(Entity entity, Args&&... args)
        {
            registry.emplace<C>(entity, std::forward<Args>(args)...);
            Broadcast(typename C::Added { {}, entity });
        }

        template <typename C>
        C* Get(Entity entity)
        {
            return registry.try_get<C>(entity);
        }

        template <typename C>
        bool Has(Entity entity)
        {
            return Get<C>(entity) != nullptr;
        }

        template <typename C, typename F>
        void Patch(Entity entity, F&& patchFunc)
        {
            registry.patch<C>(entity, patchFunc);
            Broadcast(typename C::Updated { {}, entity });
        }

        template <typename C, typename... Args>
        void Set(Entity entity, Args&&... args)
        {
            registry.replace<C>(entity, std::forward<Args>(args)...);
            Broadcast(typename C::Updated { {}, entity });
        }

        template <typename C>
        void Updated(Entity entity)
        {
            Broadcast(typename C::Updated { {}, entity });
        }

        template <typename C>
        void Remove(Entity entity)
        {
            registry.remove<C>(entity);
            Broadcast(typename C::Removed { {}, entity });
        }

        template <typename S, typename... Args>
        void EmplaceState(Args&&... args)
        {
            StateSignature signature = Internal::SignForStaticClass<S>();
            auto iter = host.states.find(signature);
            Assert(iter == host.states.end());
            host.states.emplace(std::make_pair(signature, Mirror::Any(S(std::forward<Args>(args)...))));
            Broadcast(typename S::Added {});
        }

        template <typename S>
        S* GetState()
        {
            StateSignature signature = Internal::SignForStaticClass<S>();
            auto iter = host.states.find(signature);
            if (iter == host.states.end()) {
                return nullptr;
            } else {
                return &iter->second.As<S&>();
            }
        }

        template <typename S>
        bool HasState()
        {
            return GetState<S>() != nullptr;
        }

        template <typename S, typename F>
        void PatchState(F&& patchFunc)
        {
            StateSignature signature = Internal::SignForStaticClass<S>();
            auto iter = host.states.find(signature);
            Assert(iter != host.states.end());
            patchFunc(iter->second.As<S&>());
            Broadcast(typename S::Updated {});
        }

        template <typename S, typename... Args>
        void SetState(Args&&... args)
        {
            StateSignature signature = Internal::SignForStaticClass<S>();
            auto iter = host.states.find(signature);
            Assert(iter != host.states.end());
            iter->second = S(std::forward<Args>(args)...);
            Broadcast(typename S::Updated {});
        }

        template <typename S>
        void UpdatedState()
        {
            Broadcast(typename S::Updated {});
        }

        template <typename S>
        void RemoveState()
        {
            StateSignature signature = Internal::SignForStaticClass<S>();
            auto iter = host.states.find(signature);
            Assert(iter != host.states.end());
            host.states.erase(signature);
            Broadcast(typename S::Removed {});
        }

        template <typename... C, typename... E>
        Query<entt::exclude_t<E...>, C...> StartQuery(Exclude<E...> = {})
        {
            return Query<entt::exclude_t<E...>, C...>(registry.view<C...>(entt::exclude_t<E...> {}));
        }

        template <typename E>
        void Broadcast(const E& event)
        {
            host.BroadcastEvent<E>(event);
        }

    private:
        ECSHost& host;
        entt::registry& registry;
    };
}
