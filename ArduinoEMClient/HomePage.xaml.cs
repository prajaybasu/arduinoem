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
using Microsoft.Maker.Serial;
using Newtonsoft.Json.Linq;
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

        private DeviceInformationCollection listOfDevices;
        private CancellationTokenSource ReadCancellationTokenSource;
        private MobileServiceCollection<WeatherDataItem,WeatherDataItem> data;
        private IMobileServiceSyncTable<WeatherDataItem> weatherTable = App.MobileService.GetSyncTable<WeatherDataItem>();

        public HomePage()
        {
            this.InitializeComponent();
          //  listOfDevices = new ObservableCollection<DeviceInformation>();
            ListAvailablePorts();

        }
        private async void ListAvailablePorts()
        {
            try
            {
              //  SerialDevice.GetDeviceSelectorFromUsbVidPid(0x2341, 0x0042);
                listOfDevices = await UsbSerial.listAvailableDevicesAsync();
                //listOfDevices = await DeviceInformation.listAvailableDevicesAsync(); // FromUsbVidPid(0x2341, 0x0042);
            //    listOfDevices = await BluetoothSerial.listAvailableDevicesAsync();
                    
                DeviceListSource.Source = listOfDevices;
                //   comPortInput.IsEnabled = true;
                ConnectDevices.SelectedIndex = -1;
            }
            catch (Exception ex)
            {
                  nameConnected.Text = "HERE :"+ ex.Message;
            }
        }
        private async void updateWeatherLatest()
        {
            List<WeatherDataItem> items = await App.MobileService.GetSyncTable<WeatherDataItem>().OrderByDescending(x => x.createdAt).ToListAsync();
            updateItemUI(items.First());
        }
        private void updateItemUI(WeatherDataItem latestData)
        {
            Azure_lastUpdatedAt.Text = "Last Updated At : " + latestData.createdAt.ToString();
            Azure_minHumidity.Text = "Minumum Humidity : " + latestData.humidity_min + " %";
            Azure_avgHumidity.Text = "Average Humidity : " + latestData.humidity_avg + " %";
            Azure_maxHumidity.Text = "Maximum Humidity : " + latestData.humidity_max + " %";
            Azure_maxPressure.Text = "Maximum Pressure : " + latestData.pressure_max / 100 + " hPa";
            Azure_minPressure.Text = "Minimum Pressure : " + latestData.pressure_min / 100 + " hPa";
            Azure_avgPressure.Text = "Average Pressure : " + latestData.pressure_avg / 100 + " hPa";
            Azure_minTemperature.Text = "Minimum Temperature : " + latestData.temperature_min + " °C";
            Azure_avgTemperature.Text = "Average Temperature : " + latestData.temperature_avg + " °C";
            Azure_maxTemperature.Text = "Maximum Temperature : " + latestData.temperature_max + " °C";
        }
        private async void navPivot_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if(serialPort != null)
            {
                CancelReadTask();
                CloseDevice();
            }
            if(navPivot.SelectedIndex == 0) // USB Bluetooth
            {
                nameConnected.Visibility = Visibility.Visible;
                //dataRequestState.IsEnabled = false;
                ButtonRefresh.IsEnabled = true;
                ButtonCancel.IsEnabled = false;
                ButtonConnect.IsEnabled = true;
            }
            else if(navPivot.SelectedIndex == 1)// Azure
            {
                nameConnected.Visibility = Visibility.Collapsed;
                ButtonRefresh.IsEnabled = true;
                ButtonConnect.IsEnabled = false;
                ButtonCancel.IsEnabled = false;
                await InitLocalStoreAsync(); // offline sync
                await RefreshWeatherDataItems();
                updateWeatherLatest();
            }
        }
        private void DataReceiveToggle(object sender, RoutedEventArgs e)
        {
            if(dataRequestState.IsOn && serialPort != null)
            {
                dataWriteObject = new DataWriter(serialPort.OutputStream);
                JObject dataRequest = new JObject(new JProperty("cmd", 0),new JProperty("data",1));
                WriteSerial((dataRequest.ToString(Formatting.None)));
            }
            else if (!dataRequestState.IsOn && serialPort != null)
            {
                dataWriteObject = new DataWriter(serialPort.OutputStream);
                JObject dataRequest = new JObject(new JProperty("cmd", 0), new JProperty("data", 0));
                WriteSerial((dataRequest.ToString(Formatting.None)));
            }
        }
        private async void WriteSerial(string data)
        {
            dataWriteObject = new DataWriter(serialPort.OutputStream);
            dataWriteObject.WriteString(data);
            dataWriteObject.WriteByte((byte)'\n');
            await dataWriteObject.StoreAsync();
            if (dataWriteObject != null)
            {
                dataWriteObject.DetachStream();
                dataWriteObject = null;
            }
        }
        private void ButtonSendConfig(object sender,RoutedEventArgs e)
        {
            dataWriteObject = new DataWriter(serialPort.OutputStream);
            JObject dataRequest = new JObject(new JProperty("cmd", 1),new JProperty("ssid", ssidTextBox.Text), new JProperty("sec", securityComboBox.SelectedIndex),new JProperty("password",passwordBox.Password));
            WriteSerial((dataRequest.ToString(Formatting.None)));
        }
        private async void ConnectClicked(object sender, RoutedEventArgs e)
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
                ButtonCancel.IsEnabled = true;
                ButtonConnect.IsEnabled = false;
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
                //dataRequestState.IsEnabled = true;
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
            Task<String> loadAsyncTask;
     //       uint ReadBufferLength = 2048;

            // If task cancellation was requested, comply
            cancellationToken.ThrowIfCancellationRequested();

            
            // Set InputStreamOptions to complete the asynchronous read operation when one or more bytes is available
            dataReaderObject.InputStreamOptions = InputStreamOptions.Partial;
            StreamReader sr = new StreamReader(WindowsRuntimeStreamExtensions.AsStreamForRead(serialPort.InputStream));
            // Create a task object to wait for data on the serialPort.InputStream
            loadAsyncTask = sr.ReadLineAsync();
            // Launch the task and wait
            String serialString = await loadAsyncTask;
            if (serialString.Length > 0 )
            {
                nameConnected.Text = serialString;
                //Debug.WriteLine(dataReaderObject.ReadString(bytesRead));
                //  Debug.WriteLine();//.Substring(dataReaderObject.ReadString(bytesRead).IndexOf('\0')+1));
                updateLiveData(serialString);
                Debug.WriteLine(serialString);
               // nameConnected.Text = sr.ReadLine();
               // updateLiveData(sr.ReadLine());//.Substring(dataReaderObject.ReadString(bytesRead).IndexOf('\0')+1));
               // status.Text = "bytes read successfully!";
            }
        }

        void updateLiveData (string json)
        {
           // Debug.WriteLine(json);
           WeatherDataItem LiveData = jsonToObject(json);
         //nameConnected.Text = json;
           //avgTemperature.Text = "Average Temperature : " + LiveData.temperature_avg;
           //minTemperature.Text = "Minimun Temperature until cloud data upload : " + LiveData.temperature_min;
           //maxHumidity.Text = "Max Humidity until cloud data upload : " + LiveData.humidity_max;
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
            //dataRequestState.IsEnabled = false;
            ButtonConnect.IsEnabled = true;
            ButtonCancel.IsEnabled = false;
            //sendTextButton.IsEnabled = false;
            //rcvdText.Text = "";
            if (listOfDevices != null)
            {
            //    listOfDevices.Clear()
            }
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
        private async Task InsertWeatherDataItem(WeatherDataItem weatherDataItem)
        {
            // This code inserts a new WeatherDataItem into the database. When the operation completes
            // and Mobile App backend has assigned an Id, the item is added to the CollectionView.
            await weatherTable.InsertAsync(weatherDataItem);
            data.Add(weatherDataItem);

            await SyncAsync(); // offline sync
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
            if (navPivot.SelectedIndex == 0) // USB
            {
                ListAvailablePorts();
                ButtonRefresh.IsEnabled = true;
            }
            else
            {
                await SyncAsync(); // offline sync
                await RefreshWeatherDataItems();
                updateWeatherLatest();
                ButtonRefresh.IsEnabled = true;
            }
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
