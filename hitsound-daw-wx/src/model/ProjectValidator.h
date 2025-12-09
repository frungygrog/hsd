#pragma once
#include "Track.h"
#include "Project.h"
#include <vector>
#include <string>

class ProjectValidator
{
public:
    struct ValidationError {
        double time;
        std::string message;
    };

    // Validates events and updates their ValidationState. Returns any errors found.
    static std::vector<ValidationError> Validate(Project& project);
};
