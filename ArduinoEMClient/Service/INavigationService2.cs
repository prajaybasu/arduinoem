using System;

using Windows.UI.Xaml.Controls;

using GalaSoft.MvvmLight.Views;

namespace ArduinoEMClient.Service
{
    public interface INavigationService2 : INavigationService
    {
        event EventHandler Navigated;

        Frame Root { get; set; }
    }
}
