using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using Syncfusion.UI.Xaml.Gauges;
using Windows.UI;
using Microsoft.WindowsAzure.MobileServices;
using Microsoft.WindowsAzure.MobileServices.SQLiteStore;
using Microsoft.WindowsAzure.MobileServices.Sync;
using ArduinoEMClient.DataModel;
using System.Threading.Tasks;
using Windows.UI.Popups;
using Windows.Devices.Enumeration;
using Windows.Devices.Bluetooth.Rfcomm;
using Windows.Networking.Sockets;
using System.Collections.ObjectModel;
using Windows.Storage.Streams;
using System.Threading;
using Windows.Devices.SerialCommunication;
using Newtonsoft.Json;
using System.Diagnostics;
using System.Text;
// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

namespace ArduinoEMClient
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class HomePage : Page
    {
        private SerialDevice serialPort = null;
        DataWriter dataWriteObject = null;
        DataReader dataReaderObject = null;

        private ObservableCollection<DeviceInformation> listOfDevices;
        private CancellationTokenSource ReadCancellationTokenSource;
        private MobileServiceCollection<WeatherDataItem,WeatherDataItem> data;
        private IMobileServiceSyncTable<WeatherDataItem> weatherTable = App.MobileService.GetSyncTable<WeatherDataItem>();

        public HomePage()
        {
            this.InitializeComponent();
            listOfDevices = new ObservableCollection<DeviceInformation>();
            ListAvailablePorts();

        }
        private async void ListAvailablePorts()
        {
            try
            {
                string aqs = SerialDevice.GetDeviceSelectorFromUsbVidPid(0x2341, 0x0042);
                var dis = await DeviceInformation.FindAllAsync(aqs);

                nameConnected.Text = "Select a device and connect";

                for (int i = 0; i < dis.Count; i++)
                {
                    listOfDevices.Add(dis[i]);
                }

                DeviceListSource.Source = listOfDevices;
                //   comPortInput.IsEnabled = true;
                ConnectDevices.SelectedIndex = -1;
            }
            catch (Exception ex)
            {
                  nameConnected.Text = "HERE :"+ ex.Message;
            }
        }
        private async void navPivot_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if(navPivot.SelectedIndex == 0) // USB Bluetooth
            {
                ButtonRefresh.IsEnabled = false;
                ButtonCancel.IsEnabled = true;
                ButtonConnect.IsEnabled = true;
            }
            else if(navPivot.SelectedIndex == 1)// Azure
            {
                ButtonRefresh.IsEnabled = true;
                ButtonConnect.IsEnabled = false;
                ButtonCancel.IsEnabled = false;
                await InitLocalStoreAsync(); // offline sync
                await RefreshWeatherDataItems();
            }
        }

        private async void Connect_clicked(object sender, RoutedEventArgs e)
        {
            if(DeviceType.SelectedIndex == 0) // USB
            {
               USBSerial_ConnectClicked();
            }
            else
            {
                BTSerial_ConnectClicked();
            }
        }
        private async void BTSerial_ConnectClicked()
        {

        }
        private async void USBSerial_ConnectClicked()
        {
            var selection = ConnectDevices.SelectedItems;

            if (selection.Count <= 0)
            {
                nameConnected.Text = "Select a device and connect";
                return;
            }

            DeviceInformation entry = (DeviceInformation)selection[0];

            try
            {
                serialPort = await SerialDevice.FromIdAsync(entry.Id);

                // Disable the 'Connect' button 
                //comPortInput.IsEnabled = false;
                ConnectDevices.IsEnabled = true;
                // Configure serial settings
                serialPort.WriteTimeout = TimeSpan.FromMilliseconds(1000);
                serialPort.ReadTimeout = TimeSpan.FromMilliseconds(10000);
                serialPort.IsDataTerminalReadyEnabled = true;
                serialPort.BaudRate = 115200;
                serialPort.Parity = SerialParity.None;
                serialPort.StopBits = SerialStopBitCount.One;
                serialPort.DataBits = 8;
                serialPort.Handshake = SerialHandshake.None;
                // Display configured settings
                nameConnected.Text = "Connected to " + serialPort.PortName + " at " + serialPort.BaudRate + " baud.";

                // Set the RcvdText field to invoke the TextChanged callback
                // The callback launches an async Read task to wait for data
              //  rcvdText.Text = "Waiting for data...";

                // Create cancellation token object to close I/O operations when closing the device
                ReadCancellationTokenSource = new CancellationTokenSource();

                // Enable 'WRITE' button to allow sending data
            //    sendTextButton.IsEnabled = true;

                Listen();
            }
            catch (Exception ex)
            {
                var dialog = new MessageDialog(ex.Message);
#pragma warning disable CS4014 // Because this call is not awaited, execution of the current method continues before the call is completed
                dialog.ShowAsync();
#pragma warning restore CS4014 // Because this call is not awaited, execution of the current method continues before the call is completed
                nameConnected.Text = ex.Message;
               // comPortInput.IsEnabled = true;
               // sendTextButton.IsEnabled = false;
            }
        }
        private async void Listen()
        {
            try
            {
                if (serialPort != null)
                {
                    dataReaderObject = new DataReader(serialPort.InputStream);

                    // keep reading the serial input
                    while (true)
                    {
                        await ReadAsync(ReadCancellationTokenSource.Token);
                    }
                }
            }
            catch (Exception ex)
            {
                if (ex.GetType().Name == "TaskCanceledException")
                {
                    nameConnected.Text = "Reading task was cancelled, closing device and cleaning up";
                    CloseDevice();
                }
                else
                {
                    var dialog = new MessageDialog(ex.Message);
#pragma warning disable CS4014 // Because this call is not awaited, execution of the current method continues before the call is completed
                    dialog.ShowAsync();
#pragma warning restore CS4014 // Because this call is not awaited, execution of the current method continues before the call is completed
                    nameConnected.Text = ex.Message;
                }
            }
            finally
            {
                // Cleanup once complete
                if (dataReaderObject != null)
                {
                    dataReaderObject.DetachStream();
                    dataReaderObject = null;
                }
            }
        }
        private async Task ReadAsync(CancellationToken cancellationToken)
        {
            Task<UInt32> loadAsyncTask;

            uint ReadBufferLength = 2048;

            // If task cancellation was requested, comply
            cancellationToken.ThrowIfCancellationRequested();

            // Set InputStreamOptions to complete the asynchronous read operation when one or more bytes is available
            dataReaderObject.InputStreamOptions = InputStreamOptions.Partial;

            // Create a task object to wait for data on the serialPort.InputStream
            loadAsyncTask = dataReaderObject.LoadAsync(ReadBufferLength).AsTask(cancellationToken);

            // Launch the task and wait
            UInt32 bytesRead = await loadAsyncTask;
            if (bytesRead > 0 )
            {
                //Debug.WriteLine(dataReaderObject.ReadString(bytesRead));
              //  Debug.WriteLine();//.Substring(dataReaderObject.ReadString(bytesRead).IndexOf('\0')+1));
                StringReader sr = new StringReader(Encoding.ASCII.GetString(BitConverter.GetBytes(bytesRead)));
                Debug.WriteLine(sr.ReadLine());
                updateLiveData(sr.ReadLine());//.Substring(dataReaderObject.ReadString(bytesRead).IndexOf('\0')+1));
               // status.Text = "bytes read successfully!";
            }
        }

        void updateLiveData (string json)
        {
           // Debug.WriteLine(json);
           WeatherDataItem LiveData = jsonToObject(json);
         //  nameConnected.Text = json;
           avgTemperature.Text = "Average Temperature : " + LiveData.temperature_avg;
           minTemperature.Text = "Minimun Temperature until cloud data upload : " + LiveData.temperature_min;
           maxHumidity.Text = "Max Humidity until cloud data upload : " + LiveData.humidity_max;
        }
         WeatherDataItem jsonToObject(string json)
        {
            WeatherDataItem item = null;
            // Debug.WriteLine(JsonConvert.DeserializeObject<WeatherDataItem>(json));
            try
            {
                item = JsonConvert.DeserializeObject<WeatherDataItem>(json);

            }
            catch (JsonReaderException ex)
            {
                var dialog = new MessageDialog(ex.Message);
#pragma warning disable CS4014 // Because this call is not awaited, execution of the current method continues before the call is completed
                dialog.ShowAsync();
#pragma warning restore CS4014 // Because this call is not awaited, execution of the current method continues before the call is completed

            }
            return item;
            
        }

        private void CloseDevice()
        {
            if (serialPort != null)
            {
                serialPort.Dispose();
            }
            serialPort = null;

            //comPortInput.IsEnabled = true;
            //sendTextButton.IsEnabled = false;
            //rcvdText.Text = "";
            if (listOfDevices != null)
            {
                listOfDevices.Clear();
            }
        }
      
        private async Task InsertWeatherDataItem(WeatherDataItem weatherDataItem)
        {
            // This code inserts a new WeatherDataItem into the database. When the operation completes
            // and Mobile App backend has assigned an Id, the item is added to the CollectionView.
            await weatherTable.InsertAsync(weatherDataItem);
            data.Add(weatherDataItem);

            await SyncAsync(); // offline sync
        }
        private void CancelReadTask()
        {
            if (ReadCancellationTokenSource != null)
            {
                if (!ReadCancellationTokenSource.IsCancellationRequested)
                {
                    ReadCancellationTokenSource.Cancel();
                }
            }
        }
        private void ButtonCancel_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                nameConnected.Text = "Disconnecting";
                CancelReadTask();
                CloseDevice();
                ListAvailablePorts();
            }
            catch (Exception ex)
            {
                nameConnected.Text = ex.Message;
            }
        }
        private async Task RefreshWeatherDataItems()
        {
            MobileServiceInvalidOperationException exception = null;
            try
            {
                // This code refreshes the entries in the list view by querying the TodoItems table.
                // The query excludes completed TodoItems.
                data = await weatherTable.ToCollectionAsync();
            }
            catch (MobileServiceInvalidOperationException e)
            {
                exception = e;
            }

            if (exception != null)
            {
                await new MessageDialog(exception.Message, "Error loading data").ShowAsync();
            }
        }

        private async void ButtonRefresh_Click(object sender, RoutedEventArgs e)
        {
            ButtonRefresh.IsEnabled = false;
            await SyncAsync(); // offline sync
            await RefreshWeatherDataItems();
            ButtonRefresh.IsEnabled = true;
        }


        protected override async void OnNavigatedTo(NavigationEventArgs e)
        {

                await InitLocalStoreAsync(); // offline sync
                await RefreshWeatherDataItems();
            
        }
        private async Task InitLocalStoreAsync()
        {
            if (!App.MobileService.SyncContext.IsInitialized)
            {
                var store = new MobileServiceSQLiteStore("localstore.db");
                store.DefineTable<WeatherDataItem>();
                await App.MobileService.SyncContext.InitializeAsync(store);
            }

            await SyncAsync();
        }

        private async Task SyncAsync()
        {
            await App.MobileService.SyncContext.PushAsync();
            await weatherTable.PullAsync("weatherDataItem", weatherTable.CreateQuery());
        }

    }
    public class PairedDeviceInfo
    {
        internal PairedDeviceInfo(DeviceInformation deviceInfo)
        {
            this.DeviceInfo = deviceInfo;
            this.ID = this.DeviceInfo.Id;
            this.Name = this.DeviceInfo.Name;
        }

        public string Name { get; private set; }
        public string ID { get; private set; }
        public DeviceInformation DeviceInfo { get; private set; }
    }

}
