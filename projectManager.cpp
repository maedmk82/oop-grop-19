#include "ProjectManager.h"
#include <fstream>


std::vector<Project> projects;


//---------------------------------
// ذخیره پروژه
//---------------------------------

void SaveProject(Project project)
{
    projects.push_back(project);


    // فقط 5 پروژه آخر نگه داشته شود
    if(projects.size()>5)
    {
        projects.erase(projects.begin());
    }


    // ذخیره در فایل
    std::ofstream file("projects.txt");


    for(auto p : projects)
    {
        file << p.name << " "
             << p.path << " "
             << p.pageSize
             << std::endl;
    }


    file.close();
}



//---------------------------------
// گرفتن پروژه های اخیر
//---------------------------------

std::vector<Project> GetRecentProjects()
{
    return projects;
}



//---------------------------------
// خواندن پروژه ها از فایل
//---------------------------------

std::vector<Project> LoadProjects()
{
    projects.clear();


    std::ifstream file("projects.txt");


    if(!file)
        return projects;


    Project p;


    while(file >> p.name
               >> p.path
               >> p.pageSize)
    {
        projects.push_back(p);
    }


    file.close();


    return projects;
}
