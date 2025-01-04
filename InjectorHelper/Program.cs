using System.Diagnostics;

public class Helper
{
    public static string procname = string.Empty;
    public static string dllname = string.Empty;
    public static void Main()
    {
        if (File.Exists("DLLInjector.exe"))
            Main2();
        else
        {
            Console.WriteLine("Please put the Injector in the same directory as me.");
            Console.ReadKey();
            return;
        }
    }
    public static void Main2()
    {
        Console.Clear();
        Console.Write("Please enter the Processname:");
        procname = Console.ReadLine();
        Console.Write("Please enter the DLL name (or path if not in same directory):");
        dllname = Console.ReadLine();
        Inject();
    }
    public static void Inject()
    {
        try
        {
            cmd($"""DLLInjector.exe "{procname}" {dllname}""");
            Console.ReadKey();
            return;
        }
        catch (Exception ex)
        {
            Console.WriteLine(ex.Message);
            Console.ReadLine();
            return;
        }
    }
    public static void cmd(string command)
    {
        ProcessStartInfo pro = new ProcessStartInfo("cmd.exe", "/C " + command);
        pro.WindowStyle = ProcessWindowStyle.Normal;
        pro.CreateNoWindow = false;
        Process.Start(pro);
    }
}