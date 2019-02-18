using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace FuckDPI2
{
    // Icon from https://www.emojione.com/emoji/1f595

    public partial class MainForm : Form
    {
        Process Service = null;
        
        private void ExecuteFDPISvc()
        {
            string opt = String.Format("--numThread={0} --bufferSize={1}", numThread.Value, numBufferSize.Value);
            Process p = ProcessHelper.CreateProcess(@"fdpi2svc.exe", opt, true, true);
            p.Exited += FDPISvcExited;
            p.EnableRaisingEvents = true;

            Service = p;
            Service.Start();
        }

        private void TerminateFDPISvc()
        {
            Process p = Service;
            
            if (p != null)
            {
                Service = null;
                p.EnableRaisingEvents = false;

                try
                {
                    p.StandardInput.Close();
                }
                catch
                {

                }
            }
        }

        private void FDPISvcExited(object sender, EventArgs e)
        {
            this.Invoke(new Action(() => TerminateStatus()));
        }
        
        void ExecuteStatus()
        {
            btnFuckTheInspection.Text = "STOP";
            numThread.Enabled = false;
            numBufferSize.Enabled = false;

            ExecuteFDPISvc();

            notifyIcon1.BalloonTipText = "Service is running";
            notifyIcon1.ShowBalloonTip(5);

            this.WindowState = FormWindowState.Minimized;
            this.Hide();
        }

        void TerminateStatus()
        {
            btnFuckTheInspection.Text = "FUCK THE INSPECTION";
            numThread.Enabled = true;
            numBufferSize.Enabled = true;

            TerminateFDPISvc();

            notifyIcon1.BalloonTipText = "Service is stopped";
            notifyIcon1.ShowBalloonTip(5);

            this.Show();
            this.WindowState = FormWindowState.Normal;
        }

        public MainForm()
        {
            InitializeComponent();
        }

        private void MainForm_Resize(object sender, EventArgs e)
        {
            if (this.WindowState == FormWindowState.Minimized)
            {
                this.WindowState = FormWindowState.Minimized;
                this.Hide();
            }
        }

        private void MainForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            TerminateFDPISvc();
        }

        private void notifyIcon1_MouseDoubleClick(object sender, MouseEventArgs e)
        {
            this.Show();
            this.WindowState = FormWindowState.Normal;
        }

        private void btnFuckTheInspection_Click(object sender, EventArgs e)
        {
            if (Service == null)
            {
                // Execute
                ExecuteStatus();
            } else
            {
                // Terminate
                TerminateStatus();
            }
        }
    }
}
