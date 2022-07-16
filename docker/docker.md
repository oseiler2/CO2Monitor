# Docker backend running Mosquitto InfluxDB Node-Red Grafana

## Preparation

Before running the docker compose file have a look at the various \*.env files and change the provided default passwords.
Make sure not to expose this directly to the internet without additional and read the following resources on further securing the various components

- [Grafana](https://grafana.com/docs/grafana/next/setup-grafana/configure-security/)
- [Node-Red](https://nodered.org/docs/user-guide/runtime/securing-node-red)
- [Mosquitto](https://mosquitto.org/documentation/)
- [InfluxDB](https://hub.docker.com/_/influxdb)
- Set a `credentialSecret` in [Node Red's settings.js](./nodered/settings.js#L44)

## Starting

Run `docker-compose up`

## Mosquitto

- Mosquitto is pre-configured using the [mosquitto.acl](./mosquitto/config/mosquitto.acl) and [mosquitto.passwd](./mosquitto/config/mosquitto.passwd) files. The default users/passwords are
  - `admin` // `admin`
  - `co2monitor` // `co2monitor`
  - `nodered` // `nodered`
- to change any of the above passwords use `docker exec mosquitto mosquitto_passwd -b /mosquitto/config/mosquitto.passwd {username} {password}`. Restart Mosquitto for the changes to become effective `docker restart mosquitto`

## Configure InfluxDB

### Using the UI

- Open http://127.0.0.1:8086/ and log on using the admin user and password form the [influxdb.env](influxdb.env) file
- Navigate to `Load data` -> `API tokens` and generate a `Read/Write API Token` for Node-red
  - Description `nodered`
  - Select scoped read and write access for the `co2monitors` bucket
  - the token will later be entered in a Node-red configuration node
- Create another token for Grafana
  - Description `grafana`
  - Select scoped read access for the `co2monitors` bucket
  - the token will later be entered in Grafana

### Using the CLI

- Create and note API tokens
  - Node-red `docker exec influxdb bash -c "influx auth create -o CO2Monitors -d nodered --write-bucket $(influx bucket list | grep co2monitors | cut -f 1) --read-bucket $(influx bucket list | grep co2monitors | cut -f 1) | grep nodered | cut -f 4"`
  - Grafana `docker exec influxdb bash -c "influx auth create -o CO2Monitors -d grafana --read-bucket $(influx bucket list | grep co2monitors | cut -f 1) | grep grafana | cut -f 4"`
- (optional) run `docker exec influxdb bash -c "influx auth list --user admin --hide-headers | cut -f 3"` to retrieve the admin API token

## Configure Node-Red

### Using the UI

- Open http://127.0.0.1:1880/
- Open the `Manage palette` menu to add the missing plug-ins
- Got to the `Install` tab and add the following plug-ins
  - node-red-contrib-influxdb
  - node-red-dashboard
  - node-red-node-email
- Open `Configuration nodes` in the menu and select open the `[v2.0] influxdb` node
- Enter the `nodered` API token previously created in InfluxDB
- Open the `Mosquitto` Docker configuration node and enter the Mosquitto username and password (default `nodered` // `nodered` )
- Deploy the changes using the top right Deploy menu. Deploying `Modified nodes` only is sufficient. The Mosquitto nodes should now show `connected`
- to enable email alerts and status messages configure the email node and enable it.

### Using the CLI

- Install the missing plug-ins `docker exec nodered npm install node-red-contrib-influxdb node-red-dashboard node-red-node-email`
- Restart Node-Red `docker restart nodered`
- Follow the steps above to configure the `Configuration nodes` on hte UI

### Finishing

- Once you have a CO2 monitor connected check out the Node-Red Dashboard http://127.0.0.1:1880/ui/. You might need to trigger the `Refresh monitors` flow in the `CO2 Monitors UI` tab first, or wait until it runs every hour.
- The `Get configuration` flow on the `CO2 monitors` tab runs daily and queries all monitors for their current configuration. This will show on the UI once a monitor is selected in the drop down and is also visible in the dedicated Grafana dashboard.

## Configure Grafana

- Open http://127.0.0.1:3000/ and log in using the user and password configured in [`grafana.env`](grafana.env)
- Open the `Data sources` entry from the bottom left Configuration menu and click on `InfluxDB`
- Enter the grafana token previously created in Influx
- Confirm with `Save and test`
