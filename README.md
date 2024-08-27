# AWS IoT Uteis:

### SQL como regra para salvar no banco de dados:
```
SELECT temperature, humidity, barometer,
  wind.velocity as wind_velocity,
  wind.bearing as wind_bearing
FROM 'device/+/data'
```

### SQL como regra para envio de notificações:
```
SELECT topic(2) as device_id, 
    temperature as reported_temperature, 
    30 as max_temperature 
  FROM 'device/+/data' 
  WHERE temperature > 30
```

### Json para envio de exemplo:
```
{
  "temperature": 55,
  "humidity": 80,
  "barometer": 1013,
  "wind": {
    "velocity": 22,
    "bearing": 255
  }
}
```


