#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include <string>
#include <vector>


struct Project
{
    std::string name;
    std::string path;
    std::string pageSize;
};


void LoadProjects();

void SaveProject(const Project& project);

void SaveProjectAs(const Project& project);

std::vector<Project> GetRecentProjects();

bool OpenProject(const Project& project);


#endif
