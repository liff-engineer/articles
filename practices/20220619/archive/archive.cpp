#include "archive.h"

namespace
{
    static std::map<std::string, std::unique_ptr<IArchive>(*)() >*
        gArchiveBuilders{nullptr};

    std::unique_ptr<IArchive> Create(const std::string& format) {
        return gArchiveBuilders->at(format)();
    }
}

bool IArchive::Register(const char* format, std::unique_ptr<IArchive>(*ctor)())
{
    static std::map<std::string, std::unique_ptr<IArchive>(*)()> builders{};
    if (gArchiveBuilders == nullptr) {
        gArchiveBuilders = &builders;
    }
    if (builders.find(format) != builders.end()) {
        return false;
    }
    builders[format] = ctor;
    return true;
}

void IArchive::Unregister(const char* format) {
    if (gArchiveBuilders) {
        gArchiveBuilders->erase(format);
    }
}

Archive::Archive(const char* format)
    :m_impl(Create(format)) {}
