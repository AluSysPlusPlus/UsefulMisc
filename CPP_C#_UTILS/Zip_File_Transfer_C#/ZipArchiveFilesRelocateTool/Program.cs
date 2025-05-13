/*
   TITLE    : .zip Archive File Transfer System
   AUTHOR   : Alushi
   DATE     : 2025
   VERSION  : 0.1

   RELEASE NOTE:
   Windows system program to transfer .zip files (or other defined extension)
   from a specified source folder to a specified destination folder.
   When the number of files in the destination exceeds a configured maximum (n),
   it keeps only the latest (m) files and deletes the older ones.
*/

// Import required namespaces
using System;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using Microsoft.Extensions.Configuration;

// Define the main class 'Program'
class Program
{
    // Fields to hold paths, extension, and parameters from configuration
    private static string? _sourceFolder;
    private static string? _destinationFolder;
    private static string? _extension;
    private static int _maxFilesInDestination;
    private static int _filesToKeep;

    // Entry point of the application
    static async Task Main(string[] args)
    {
        // Load configuration from appsettings.json
        LoadConfiguration();

        // Ensure source and destination directories exist (create if missing)
        EnsureDirectoriesExist();

        // Initialize a FileSystemWatcher to monitor new files with the specified extension
        using var watcher = new FileSystemWatcher(_sourceFolder!, $"*{_extension}")
        {
            NotifyFilter = NotifyFilters.FileName | NotifyFilters.CreationTime,
            EnableRaisingEvents = true
        };

        // Subscribe to the Created event (triggered when new file appears)
        watcher.Created += OnNewFileDetected;

        // Console info output
        Console.WriteLine(" File Watcher started!");
        Console.WriteLine($" Watching folder: {_sourceFolder}");
        Console.WriteLine($" Files will be copied to: {_destinationFolder}");
        Console.WriteLine($" Monitoring file type: {_extension}");
        Console.WriteLine(" Press [Enter] to exit.");

        // Keep the app running until user hits Enter
        await Task.Run(() => Console.ReadLine());
    }

    // Load and validate configuration values from appsettings.json
    private static void LoadConfiguration()
    {
        IConfiguration config = new ConfigurationBuilder()
            .SetBasePath(Directory.GetCurrentDirectory())
            .AddJsonFile("appsettings.json", optional: false)
            .Build();

        _sourceFolder = config["SourceFolder"];
        _destinationFolder = config["DestinationFolder"];
        _extension = config["Extension"];

        if (string.IsNullOrEmpty(_sourceFolder) ||
            string.IsNullOrEmpty(_destinationFolder) ||
            string.IsNullOrEmpty(_extension))
        {
            throw new Exception("One or more configuration values are missing!");
        }

        // Load optional numeric settings with fallback defaults
        _maxFilesInDestination = int.TryParse(config["MaxFilesInDestination"], out var max) ? max : 10;
        _filesToKeep = int.TryParse(config["FilesToKeep"], out var keep) ? keep : 3;
    }

    // Create the source and destination directories if they don't exist
    private static void EnsureDirectoriesExist()
    {
        Directory.CreateDirectory(_sourceFolder!);
        Directory.CreateDirectory(_destinationFolder!);
    }

    // Event handler: triggered when a new matching file appears in the source folder
    private static async void OnNewFileDetected(object sender, FileSystemEventArgs e)
    {
        var fileName = Path.GetFileName(e.FullPath);
        Console.WriteLine($" New file detected: {fileName}");

        try
        {
            // Wait for file to be fully written before copying
            await WaitForFileReady(e.FullPath);

            // Copy the file to the destination
            var destinationPath = Path.Combine(_destinationFolder!, fileName);
            File.Copy(e.FullPath, destinationPath, true);
            Console.WriteLine($" Copied to: {destinationPath}");

            // Clean up the destination folder if too many files exist
            CleanupDestination();
        }
        catch (Exception ex)
        {
            Console.WriteLine($" Error processing file '{fileName}': {ex.Message}");
        }
    }

    // Delete old files in destination to keep it within allowed limits
    private static void CleanupDestination()
    {
        var files = new DirectoryInfo(_destinationFolder!)
            .GetFiles($"*{_extension}")
            .OrderBy(f => f.CreationTime) // Oldest files first
            .ToList();

        // Delete oldest files if the count exceeds the allowed max
        if (files.Count >= _maxFilesInDestination)
        {
            var filesToDelete = files.Take(files.Count - _filesToKeep);
            foreach (var file in filesToDelete)
            {
                try
                {
                    file.Delete();
                    Console.WriteLine($" Deleted old file: {file.Name}");
                }
                catch (Exception ex)
                {
                    Console.WriteLine($" Could not delete '{file.Name}': {ex.Message}");
                }
            }
        }
    }

    // Wait until the file is fully written and ready for copying
    private static async Task WaitForFileReady(string filePath)
    {
        bool ready = false;

        while (!ready)
        {
            try
            {
                // Try to open file exclusively - if successful, it's ready
                using var stream = File.Open(filePath, FileMode.Open, FileAccess.Read, FileShare.None);
                ready = true;
            }
            catch (IOException)
            {
                // If the file is still being written, wait and retry
                await Task.Delay(500);
            }
        }
    }
}
