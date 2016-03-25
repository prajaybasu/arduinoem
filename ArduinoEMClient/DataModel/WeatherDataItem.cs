using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace ArduinoEMClient.DataModel
{
    class WeatherDataItem
    {
        //Units
        // Temperature : Degrees Celcius
        // Relative Humidity : no unit, percentage
        // Pressure : library default, pascal : Pa (N/m^2)
        // MQx : raw analog value from Arduino analogRead(),
        // lux : lux (adafruit)
        //ml8511 : mw/cm^2
        public string Id { get; set; }
        //TODO : Add vars from table

        [Microsoft.WindowsAzure.MobileServices.CreatedAt]
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public DateTime createdAt { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double mq2_min { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double mq2_avg { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double mq2_max { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double mq3_min { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double mq3_avg { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double mq3_max { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double mq4_min { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double mq4_avg { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double mq4_max { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double mq5_min { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double mq5_avg { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double mq5_max { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double mq6_min { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double mq6_avg { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double mq6_max { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double mq7_min { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double mq7_avg { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double mq7_max { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double mq8_min { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double mq8_avg { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double mq8_max { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double mq9_min { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double mq9_avg { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double mq9_max { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double mq135_min { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double mq135_avg { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double mq135_max { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double temperature_min { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double temperature_avg { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double temperature_max { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double humidity_min { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double humidity_avg { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double humidity_max { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double lux_min { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double lux_avg { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double lux_max { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double uvb_min { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double uvb_avg { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double uvb_max { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public string err { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double pressure_min { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double pressure_max { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double pressure_avg { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double dust_min { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double dust_max { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double dust_avg { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double dust1_min { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double dust1_max { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double dust1_avg { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double dust25_min { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double dust25_max { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public double dust25_avg { get; set; }
        [JsonProperty(NullValueHandling = NullValueHandling.Ignore)]
        public string deviceId { get; set; }
    }
}
