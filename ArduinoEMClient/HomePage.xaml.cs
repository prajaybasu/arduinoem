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
// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

namespace ArduinoEMClient
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class HomePage : Page
    {
        private MobileServiceCollection<WeatherDataItem,WeatherDataItem> data;
        private IMobileServiceSyncTable<WeatherDataItem> weatherTable = App.MobileService.GetSyncTable<WeatherDataItem>();

        public HomePage()
        {
            this.InitializeComponent();
           // updateTemperatureGuage(40);
        }
        private async Task InsertWeatherDataItem(WeatherDataItem weatherDataItem)
        {
            // This code inserts a new WeatherDataItem into the database. When the operation completes
            // and Mobile App backend has assigned an Id, the item is added to the CollectionView.
            await weatherTable.InsertAsync(weatherDataItem);
            data.Add(weatherDataItem);

            await SyncAsync(); // offline sync
        }
        private async Task RefreshTodoItems()
        {
            MobileServiceInvalidOperationException exception = null;
            try
            {
                // This code refreshes the entries in the list view by querying the TodoItems table.
                // The query excludes completed TodoItems.
                data = await weatherTable
                    .ToCollectionAsync();
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
            await RefreshTodoItems();

            ButtonRefresh.IsEnabled = true;
        }


        protected override async void OnNavigatedTo(NavigationEventArgs e)
        {
            await InitLocalStoreAsync(); // offline sync
            await RefreshTodoItems();
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
        void updateTemperatureGuage(float temperature)
        {
            temperaturePointer.Value = temperature;
            if(temperature < 18 && temperature > -20)
            {
                temperaturePointer.BarPointerStroke = new SolidColorBrush(Colors.Blue);
            }
            else if (temperature > 18 && temperature < 25)
            {
                temperaturePointer.BarPointerStroke = new SolidColorBrush(Colors.Green);
            }
            else if(temperature > 25 && temperature < 35)
            {
                temperaturePointer.BarPointerStroke = new SolidColorBrush(Colors.OrangeRed);
            }
            else if(temperature < 40 && temperature > 35)
            {
                temperaturePointer.BarPointerStroke = new SolidColorBrush(Colors.Red);
            }
            else
            {
                temperaturePointer.BarPointerStroke = new SolidColorBrush(Colors.DarkRed);
            }
            temperatureText.Text = temperature + " °C";
        }

    }
}
