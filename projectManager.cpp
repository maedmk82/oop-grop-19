#include "ProjectManager.h"

#include <fstream>
#include <iostream>
#include <vector>
#include <string>


// ------------------------------------
// حداکثر تعداد پروژه‌های اخیر
// ------------------------------------
const int MAX_RECENT_PROJECTS = 10;

// ------------------------------------
// فایل ذخیره پروژه‌های اخیر
// ------------------------------------
const std::string PROJECT_FILE = "recent_projects.dat";

// ------------------------------------
// لیست پروژه‌ها در حافظه رم
// ------------------------------------
std::vector<Project> projects;


// ------------------------------------
// ذخیره لیست پروژه‌ها روی هارد (با فرمت پایپ |)
// ------------------------------------
static void SaveRecentProjectsToFile()
{
    std::ofstream file(PROJECT_FILE, std::ios::trunc);

    if(!file.is_open())
    {
        std::cout << "Cannot save recent projects." << std::endl;
        return;
    }

    for(const Project& project : projects)
    {
        file << project.name << "|"
             << project.path << "|"
             << project.pageSize << "\n";
    }

    file.close();
}


// ------------------------------------
// خواندن پروژه‌ها از فایل هنگام اجرای برنامه
// ------------------------------------
void LoadProjects()
{
    projects.clear();

    std::ifstream file(PROJECT_FILE);

    if(!file.is_open())
    {
        return;
    }

    std::string name;
    std::string path;
    std::string pageSize;

    // خواندن خطوط با جداکننده '|'
    while(
        std::getline(file, name, '|') &&
        std::getline(file, path, '|') &&
        std::getline(file, pageSize)
    )
    {
        Project project;
        project.name = name;
        project.path = path;
        project.pageSize = pageSize;

        projects.push_back(project);
    }

    file.close();

    // فقط 10 پروژه نگه داشته شود
    if(projects.size() > MAX_RECENT_PROJECTS)
    {
        projects.resize(MAX_RECENT_PROJECTS);
    }
}


// ------------------------------------
// اضافه کردن پروژه به Recent (مغز متفکر مدیریت لیست)
// ------------------------------------
static void AddToRecentProjects(const Project& project)
{
    // اگر همین پروژه قبلاً در لیست بوده، نسخه قدیمی را پاک کن
    for(auto it = projects.begin(); it != projects.end(); ++it)
    {
        if(it->name == project.name)
        {
            projects.erase(it);
            break;
        }
    }

    // پروژه جدید را در اول (بالای) لیست قرار بده
    projects.insert(projects.begin(), project);

    // فقط 10 پروژه اخیر نگهداری شود
    if(projects.size() > MAX_RECENT_PROJECTS)
    {
        projects.resize(MAX_RECENT_PROJECTS);
    }

    // ذخیره تغییرات روی هارد دیسک
    SaveRecentProjectsToFile();
}


// ------------------------------------
// Save Project
// ------------------------------------
// ------------------------------------
// Save Project
// ------------------------------------
void SaveProject(const Project& p) // <--- کلمه const و علامت & اضافه شد
{
    AddToRecentProjects(p);
}


// ------------------------------------
// Get Recent Projects
// ------------------------------------
std::vector<Project> GetRecentProjects()
{
    // لیست خوانده شده را برمی‌گرداند
    return projects;
}


// ------------------------------------
// Save As
// ------------------------------------
void SaveProjectAs(const Project& project)
{
    AddToRecentProjects(project);

    std::cout << "Project Saved As: " << project.name << std::endl;
}


// ------------------------------------
// Open Project
// ------------------------------------
bool OpenProject(const Project& project)
{
    std::cout << "Opening Project: " << project.name << std::endl;

    // وقتی پروژه باز می‌شود، آن را به ابتدای Recent منتقل کن
    AddToRecentProjects(project);

    return true;
}
