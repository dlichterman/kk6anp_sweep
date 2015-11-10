using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO;
using System.Windows.Forms.DataVisualization.Charting;

/*
 * Copyright  Attribution-ShareAlike 4.0 International (CC BY-SA 4.0)
 * http://creativecommons.org/licenses/by-sa/4.0/
 * 
 * See readme for more information
 * 
 * */

namespace Sweeper
{
    public partial class MainForm : Form
    {
        List<ResponseClass> list;
        List<string> strlist;
        Point? prevPosition;
        public MainForm()
        {
            InitializeComponent();
            list = new List<ResponseClass>();
            strlist = new List<string>();
            prevPosition = null;
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            refreshComPorts();
            cbBand.SelectedIndex = 0;
        }

        private void button1_Click(object sender, EventArgs e)
        {
            //refresh the com ports
            if(serialPort.IsOpen)
            {
                serialPort.Close();
                btnSweep.Enabled = false;
                btnSaveCSV.Enabled = false;
            }
            refreshComPorts();
        }

        private void refreshComPorts()
        {
            cbComPort.Items.Clear();
            string[] coms = System.IO.Ports.SerialPort.GetPortNames();
            foreach (string s2 in coms)
            {
                cbComPort.Items.Add(s2);
            }

            if (cbComPort.Items.Count > 0)
            {
                cbComPort.SelectedIndex = 0;
                btnConnect.Enabled = true;
            }
            else
            {
                btnConnect.Enabled = false;
            }
            
        }

        private void btnConnect_Click(object sender, EventArgs e)
        {
            if(cbComPort.Text != "")
            {
                serialPort.BaudRate = 57600;
                serialPort.PortName = cbComPort.Text;
                serialPort.DataBits = 8;
                serialPort.Parity = System.IO.Ports.Parity.None;
                serialPort.StopBits = System.IO.Ports.StopBits.One;
                serialPort.RtsEnable = true;
                serialPort.DtrEnable = true;

                try
                {
                    serialPort.Open();
                    if(serialPort.IsOpen)
                    {
                        btnConnect.Enabled = false;
                        btnSweep.Enabled = true;
                        btnSaveCSV.Enabled = true;
                        //serialPort.WriteLine("?");
                    }
                }
                catch(Exception e2)
                {
                    MessageBox.Show(e2.Message);
                }
            }
        }

        private void serialPort_DataReceived(object sender, System.IO.Ports.SerialDataReceivedEventArgs e)
        {
            
            try
            {
                string line = serialPort.ReadLine();
                strlist.Add(line);

                if(line.Contains("Start") || line.Contains("Stop") || line.Contains("Num"))
                {
                    return;
                }

                if(line.Contains("End"))
                {
                    plotPoints();
                }
                else
                {
                    string[] lineitems = line.Split(',');
                    ResponseClass r = new ResponseClass();
                    r.Freq = (double.Parse(lineitems[0]) / 1000000);
                    r.VSWR = (double.Parse(lineitems[1]) / 1000);
                    list.Add(r);
                    
                }


            }
            catch(Exception ex)
            {

            }
            

        }

        delegate void SetCallback();

        private void plotPoints()
        {
            if (this.chartSWR.InvokeRequired)
            {
                SetCallback d = new SetCallback(plotPoints);
                this.Invoke(d);
            }
            else
            {
                listBoxResults.Items.Clear();
                foreach(string s in strlist)
                {
                    string[] s1 = s.Split(',');
                    if (s1.Count() > 1)
                    {
                        listBoxResults.Items.Add( Math.Round(double.Parse(s1[0]) / 1000000,6).ToString("0.000000") + "," + Math.Round(double.Parse(s1[1]) / 1000,6).ToString("0.0000"));
                    }

                }
                //listBox1.Items.AddRange(strlist.ToArray());
                chartSWR.ChartAreas["ChartArea1"].AxisY.Maximum = 10;
                foreach (ResponseClass l in list)
                {
                    chartSWR.Series["VSWR"].Points.AddXY(l.Freq, l.VSWR);
                }
                chartSWR.Series["VSWR"].ChartArea = "ChartArea1";
                btnSweep.Enabled = true;
                btnSaveCSV.Enabled = true;
                btnSweep.Text = "Sweep";
            }
        }

        private void btnSweep_Click(object sender, EventArgs e)
        {
            btnSweep.Enabled = false;
            btnSaveCSV.Enabled = false;
            btnSweep.Text = "Sweeping...";
            list.Clear();
            chartSWR.Series["VSWR"].Points.Clear();
            listBoxResults.Items.Clear();
            strlist.Clear();
            
            double fStart = double.Parse(tbStart.Text);
            double fStop = double.Parse(tbStop.Text);

            int numSteps = int.Parse(cbSteps.Text);


            serialPort.WriteLine((fStart * 1000000).ToString() + "A" + "\n");//Set start frequency
            System.Threading.Thread.Sleep(300);
            serialPort.WriteLine((fStop * 1000000).ToString() + "B" + "\n"); //Set stop frequency
            System.Threading.Thread.Sleep(300);
            serialPort.WriteLine(numSteps + "N" + "\n");  //Set number of steps
            System.Threading.Thread.Sleep(300);
            //Start the sweep
            serialPort.WriteLine("S" + "\n");
        }

        private void chart1_MouseMove(object sender, MouseEventArgs e)
        {
            var pos = e.Location;
            if (prevPosition.HasValue && pos == prevPosition.Value)
                return;
            toolTip.RemoveAll();
            prevPosition = pos;
            var results = chartSWR.HitTest(pos.X, pos.Y, false,
                                            ChartElementType.DataPoint);
            foreach (var result in results)
            {
                if (result.ChartElementType == ChartElementType.DataPoint)
                {
                    var prop = result.Object as DataPoint;
                    if (prop != null)
                    {
                        var pointXPixel = result.ChartArea.AxisX.ValueToPixelPosition(prop.XValue);
                        var pointYPixel = result.ChartArea.AxisY.ValueToPixelPosition(prop.YValues[0]);

                        // check if the cursor is really close to the point (2 pixels around the point)
                        if (Math.Abs(pos.X - pointXPixel) < 2 &&
                            Math.Abs(pos.Y - pointYPixel) < 2)
                        {
                            toolTip.Show("Frequency: " + prop.XValue + " MHz, SWR: " + prop.YValues[0], this.chartSWR,
                                            pos.X, pos.Y - 15);
                        }
                    }
                }
            }
        }

        private void cbBand_SelectedIndexChanged(object sender, EventArgs e)
        {
            switch(cbBand.SelectedIndex)
            {
                case 0:
                    tbStart.Text = "1";
                    tbStop.Text = "30";
                    break;
                case 1:
                    tbStart.Text = "1";
                    tbStop.Text = "10";
                    break;
                case 2: 
                    tbStart.Text = "10";
                    tbStop.Text = "20";
                    break;
                case 3:
                    tbStart.Text = "20";
                    tbStop.Text = "30";
                    break;
                case 4: //10m
                    tbStart.Text = "27.9";
                    tbStop.Text = "29.8";
                    break;
                case 5: //12m
                    tbStart.Text = "24.8";
                    tbStop.Text = "25";
                    break;
                case 6: //15m
                    tbStart.Text = "20.9";
                    tbStop.Text = "21.55";
                    break;
                case 7: //17m
                    tbStart.Text = "18";
                    tbStop.Text = "18.2";
                    break;
                case 8: //20m
                    tbStart.Text = "13.9";
                    tbStop.Text = "14.45";
                    break;
                case 9: //30m
                    tbStart.Text = "10";
                    tbStop.Text = "10.25";
                    break;
                case 10: //40m
                    tbStart.Text = "6.9";
                    tbStop.Text = "7.4";
                    break;
                case 11: //60m
                    tbStart.Text = "5.2";
                    tbStop.Text = "5.5";
                    break;
                case 12: //80m
                    tbStart.Text = "3.4";
                    tbStop.Text = "4.1";
                    break;
                case 13: //160m
                    tbStart.Text = "1.7";
                    tbStop.Text = "2.1";
                    break;
            }
        }

        private void btnSaveCSV_Click(object sender, EventArgs e)
        {
            //save file from listbox
            try
            {
                if (saveDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
                {
                    StreamWriter sw = new StreamWriter(saveDialog.FileName);
                    sw.WriteLine("Frequency,SWR");
                    foreach (string s in listBoxResults.Items)
                    {
                        sw.WriteLine(s);
                    }
                    sw.Close();

                    System.Diagnostics.Process.Start(saveDialog.FileName);
                }
            }
            catch(Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }

        private void exitToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        private void aboutToolStripMenuItem_Click(object sender, EventArgs e)
        {
            AboutBox b = new AboutBox();
            b.ShowDialog();

        }

        private void moreInfoToolStripMenuItem_Click(object sender, EventArgs e)
        {
            System.Diagnostics.Process.Start("https://github.com/dlichterman/kk6anp_sweep");
        }

    }
}
