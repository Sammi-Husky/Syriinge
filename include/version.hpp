#pragma once

#include <stdio.h>

#define SYRINGE_VERSION "0.6.0"

class Version {
public:
    Version(const char* versionStr)
    {
        sscanf(versionStr, "%d.%d.%d", &this->major, &this->minor, &this->revision);
    }
    friend bool operator<(const Version& lh, const Version& rh)
    {
        if (rh.major > lh.major)
            return true;

        if (rh.minor > lh.minor)
            return true;

        if (rh.revision > lh.revision)
            return true;

        return false;
    }
    friend bool operator==(const Version& lh, const Version& rh)
    {
        return lh.major == rh.major && lh.minor == rh.minor && lh.revision == rh.revision;
    }
    friend bool operator!=(const Version& lh, const Version& rh)
    {
        return lh.major != rh.major || lh.minor != rh.minor || lh.revision != rh.revision;
    }

    void toString(const Version& version, char* buffer)
    {
        sprintf(buffer, "%d.%d.%d", version.major, version.minor, version.revision);
    }
    char major, minor, revision;
};