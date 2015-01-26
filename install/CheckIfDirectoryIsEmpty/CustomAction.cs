using System;
using System.Collections.Generic;
using System.Text;
using Microsoft.Deployment.WindowsInstaller;
using System.IO;


// From http://stackoverflow.com/questions/17533239/wix-checking-installdir-is-empty
namespace CheckIfDirectoryIsEmpty
{
    public class CustomActions
    {
        [CustomAction]
        public static ActionResult CheckInstalFolderEmptyness(Session session)
        {
            session.Log("CheckDirectory Emptyness");

            string installFolder = session["INSTALLFOLDER"];
            installFolder = installFolder.Trim();
            session["InstallDirOk"] = "1";
            if (Directory.Exists(installFolder) && Directory.GetFiles(installFolder, "*", SearchOption.AllDirectories).Length > 0)
            {
                session["InstallDirOk"] = "0";
            }

            return ActionResult.Success;
        }
    }
}
