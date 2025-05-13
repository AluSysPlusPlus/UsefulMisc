# 📦 .zip Archive File Transfer System

**Author**: Alushi  
**Version**: 0.1  
**Release Year**: 2025  

---

## 📄 Description

A lightweight and configurable file-watching utility for Windows, built in **C#**, which continuously monitors a specified folder for new `.zip` files (or any extension you choose).  
When a new file appears, it is automatically copied to a destination folder. The destination folder is then managed: once the file count exceeds a configured limit, the oldest files are deleted, keeping only the most recent ones.

This tool is ideal for automated workflows where only the latest files are needed -- such as temporary backups, monitored image dumps, or job queues.

---

## 🛠️ Features

- 🔍 Monitors a folder for new files of a specified type
- 📁 Automatically copies new files to a destination folder
- ♻️ Manages the destination folder by removing older files when a limit is exceeded
- 📄 Fully configurable through `appsettings.json`
- ✅ Works silently in the background (can be adapted into a Windows Service)

---

## 📂 Folder Structure

