﻿/*
 * TCP Port Listener & Tester
 * Author: Alushi
 * Description:
 *   - Starts TCP listeners on configured ports.
 *   - Allows runtime commands to start/stop listeners and test connectivity.
 *   - Useful for simulating open ports and testing clients.
 */

using System;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Threading.Tasks;

class Program
{
    // Store active TCP listeners per port
    static Dictionary<int, TcpListener> listeners = new Dictionary<int, TcpListener>();

    // Track cancellation tokens to stop listener tasks cleanly
    static Dictionary<int, CancellationTokenSource> tokens = new Dictionary<int, CancellationTokenSource>();

    // Port configuration: port number -> label
    static Dictionary<int, string> portConfig = new Dictionary<int, string>
    {
        { 7129, "CLS" },
        { 7130, "OCR" },
        { -1, "-1" } // Disabled entry
    };

    static void Main()
    {
        // Automatically start listeners for all active ports
        foreach (var entry in portConfig)
        {
            int port = entry.Key;
            string label = entry.Value;
            if (port > 0)
            {
                StartListener(port, label);
            }
        }

        // Show available commands
        Console.WriteLine("Commands:");
        Console.WriteLine("  start <port>   - Start listening on a port");
        Console.WriteLine("  stop <port>    - Stop listening on a port");
        Console.WriteLine("  test <port>    - Test socket connection to port");
        Console.WriteLine("  test all       - Test all configured ports");
        Console.WriteLine("  exit           - Exit program");

        // Command input loop
        while (true)
        {
            var cmd = Console.ReadLine();
            if (cmd == null) continue;

            if (cmd.StartsWith("stop "))
            {
                if (int.TryParse(cmd.Substring(5), out int port))
                {
                    StopListener(port);
                }
            }
            else if (cmd.StartsWith("start "))
            {
                if (int.TryParse(cmd.Substring(6), out int port))
                {
                    StartListener(port, "MANUAL");
                }
            }
            else if (cmd.StartsWith("test all"))
            {
                // Test all configured ports
                foreach (var entry in portConfig)
                {
                    int port = entry.Key;
                    if (port > 0)
                    {
                        bool result = TestSocketConnection("127.0.0.1", port);
                        Console.WriteLine(result
                            ? $"[✓] Connection to port {port} succeeded."
                            : $"[✗] Connection to port {port} failed.");
                    }
                }
            }
            else if (cmd.StartsWith("test "))
            {
                if (int.TryParse(cmd.Substring(5), out int port))
                {
                    bool result = TestSocketConnection("127.0.0.1", port);
                    Console.WriteLine(result
                        ? $"[✓] Connection to port {port} succeeded."
                        : $"[✗] Connection to port {port} failed.");
                }
            }
            else if (cmd == "exit")
            {
                // Gracefully stop all listeners
                foreach (var port in new List<int>(listeners.Keys))
                    StopListener(port);
                break;
            }
        }
    }

    // Start TCP listener on a specific port
    static void StartListener(int port, string label)
    {
        if (listeners.ContainsKey(port))
        {
            Console.WriteLine($"[!] Port {port} already running.");
            return;
        }

        try
        {
            var listener = new TcpListener(IPAddress.Loopback, port);
            listener.Start();
            listeners[port] = listener;

            var tokenSource = new CancellationTokenSource();
            tokens[port] = tokenSource;

            // Run the listener loop in a background task
            Task.Run(() => ListenLoop(listener, port, label, tokenSource.Token));
            Console.WriteLine($"[+] Listening on port {port} ({label})");
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[X] Failed to start port {port}: {ex.Message}");
        }
    }

    // Stop TCP listener on a specific port
    static void StopListener(int port)
    {
        if (listeners.ContainsKey(port))
        {
            tokens[port].Cancel();           // Cancel the listener loop
            listeners[port].Stop();          // Close the socket
            listeners.Remove(port);          // Remove from dictionaries
            tokens.Remove(port);
            Console.WriteLine($"[-] Stopped listening on port {port}");
        }
        else
        {
            Console.WriteLine($"[!] Port {port} not running.");
        }
    }

    // Listener task: handles incoming connections
    static async Task ListenLoop(TcpListener listener, int port, string label, CancellationToken token)
    {
        try
        {
            while (!token.IsCancellationRequested)
            {
                // Accept client if there's a pending connection
                if (listener.Pending())
                {
                    var client = await listener.AcceptTcpClientAsync();
                    Console.WriteLine($"[{port}] Connection from {client.Client.RemoteEndPoint}");
                    client.Close(); // Close immediately (just test connection)
                }

                await Task.Delay(100); // Avoid busy loop
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[X] Listener on port {port} error: {ex.Message}");
        }
    }

    // Test a socket connection to the given IP and port with timeout
    static bool TestSocketConnection(string ip, int port, int timeout = 2000)
    {
        try
        {
            using (var client = new TcpClient())
            {
                var result = client.BeginConnect(ip, port, null, null);
                bool success = result.AsyncWaitHandle.WaitOne(timeout);
                if (!success) return false;

                client.EndConnect(result);
                return true;
            }
        }
        catch
        {
            return false;
        }
    }
}
