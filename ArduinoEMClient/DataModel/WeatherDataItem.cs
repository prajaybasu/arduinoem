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
        // Pressure : library default, pascal : Pa
        // MQx : raw analog value from Arduino analogRead(),
        // lux : lux (adafruit)
        public string Id { get; set; }
        //TODO : Add vars from table

        [Microsoft.WindowsAzure.MobileServices.CreatedAt]
        public DateTime createdAt { get; set; }
        [JsonProperty(PropertyName = "temperature")]
        public float temperature { get; set; }
        [JsonProperty(PropertyName = "pressure")]
        public float pressure { get; set; }
        [JsonProperty(PropertyName = "humidity")]
        public int humidity { get; set; }
        [JsonProperty(PropertyName = "lux")]
        public int lux { get; set; }
        [JsonProperty(PropertyName = "mq2")]
        public int mq2 { get; set; }
        [JsonProperty(PropertyName = "mq3")]
        public int mq3 { get; set; }
        [JsonProperty(PropertyName = "mq4")]
        public int mq4 { get; set; }
        [JsonProperty(PropertyName = "mq5")]
        public int mq5 { get; set; }
        [JsonProperty(PropertyName = "mq6")]
        public int mq6 { get; set; }
        [JsonProperty(PropertyName = "mq7")]
        public int mq7 { get; set; }
        [JsonProperty(PropertyName = "mq8")]
        public int mq8 { get; set; }
        [JsonProperty(PropertyName = "mq9")]
        public int mq9 { get; set; }
        [JsonProperty(PropertyName = "mq135")]
        public int mq135 { get; set; }

    }
}
