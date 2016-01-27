#pragma once
#include <iscore/tools/Metadata.hpp>
#include <Process/ProcessFactoryKey.hpp>
#define PROCESS_FACTORY_METADATA(Export, Model, Uuid) \
template<> \
struct Export Metadata< \
        ConcreteFactoryKey_k, \
        Model> \
{ \
        static const auto& get() \
        { \
            static const ProcessFactoryKey k{Uuid}; \
            return k; \
        } \
};

#define PROCESS_METADATA(Export, Model, Uuid, ObjectKey, PrettyName) \
 OBJECTKEY_METADATA(Export, Model, ObjectKey) \
 PROCESS_FACTORY_METADATA(Export, Model, Uuid) \
template<> \
struct Export Metadata< \
        PrettyName_k, \
        Model> \
{ \
        static auto get() \
        { \
            return QObject::tr(PrettyName); \
        } \
};