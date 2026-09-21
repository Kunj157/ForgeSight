# ForgeSight Device Simulators

Python simulators that publish sensor readings over MQTT. Default mode is
synthetic Gaussian noise (with optional anomaly spikes). Pass `--replay` to
walk a NASA C-MAPSS or UCI SECOM trace instead, scaled onto the ranges in
`config.yaml`.

```bash
# synthetic (what scripts/dev-up.sh runs)
PYTHONPATH=. python3 -m simulators.run -c simulators/config.yaml

# C-MAPSS / SECOM — fetch into gitignored data/ first
./scripts/fetch-replay-data.sh
PYTHONPATH=. python3 -m simulators.run -c simulators/config.yaml \
  --replay cmapss --replay-file data/cmapss/train_FD001.txt
PYTHONPATH=. python3 -m simulators.run -c simulators/config.yaml \
  --replay secom --replay-file data/secom/secom.data \
  --replay-labels data/secom/secom_labels.data
```

`--no-replay-loop` holds the last sample when the trace ends (default: loop).

## Column mapping

C-MAPSS is 26 space-separated columns (unit, cycle, 3 settings, 21 sensors).
Units in file order are assigned to configured devices. Late-life cycles
(last 20% of each unit) are flagged `anomaly`.

| Sensor        | C-MAPSS column | Notes                          |
| ------------- | -------------- | ------------------------------ |
| temperature   | 6 (T24)        | LPC outlet temperature         |
| pressure      | 11 (P30)       | HPC outlet pressure            |
| vibration     | 8 (T50)        | wear-correlated proxy          |
| flow          | 16 (phi)       | fuel-flow equivalent           |
| current       | 20 (W31)       | coolant bleed → electrical load |

SECOM is 590 anonymous process features. The first five columns map onto the
same sensor names in the table above. A labels file whose first token is `1`
marks that wafer as `anomaly`.

A device only receives sensors it actually has in `config.yaml`. Values are
min-max scaled into each sensor's `[min, max]`.

## Tests

```bash
PYTHONPATH=. python3 -m pytest simulators/ -v
```
