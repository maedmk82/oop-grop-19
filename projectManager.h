#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include <string>
#include <vector>


struct Project
{
    std::string name;
    std::string path;
    int pageSize;
};


// ذخیره پروژه جدید
void SaveProject(Project project);


// گرفتن 5 پروژه آخر
std::vector<Project> GetRecentProjects();


// خواندن همه پروژه های ذخیره شده
std::vector<Project> LoadProjects();


#endif
