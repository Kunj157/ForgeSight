#include "ui/plant_layout.h"

#include <QMap>

#include <algorithm>
#include <cmath>

namespace ui {

float PlantLayout::normalise(const QString& sensor, double value) {
    auto it = ranges_.find(sensor);
    if (it == ranges_.end()) {
        it = ranges_.insert(sensor, {value, value});
    } else {
        it->first = std::min(it->first, value);
        it->second = std::max(it->second, value);
    }
    const double span = it->second - it->first;
    const double t = span > 1e-9 ? (value - it->first) / span : 0.5;
    return static_cast<float>(0.15 + 0.85 * std::clamp(t, 0.0, 1.0));
}

PlantScene PlantLayout::layout(const QVector<PlantDevice>& input) {
    PlantScene scene;
    if (input.isEmpty())
        return scene;

    // plant -> floor -> devices, all sorted for stable placement.
    QMap<QString, QMap<QString, QVector<PlantDevice>>> groups;
    for (const auto& d : input)
        groups[d.plant][d.floor].push_back(d);

    float plant_x = 0.0f;
    for (auto plant = groups.begin(); plant != groups.end(); ++plant) {
        // Size every zone of this plant first: the plant column is as wide as
        // its widest floor.
        struct Sized {
            QString floor;
            QVector<PlantDevice> devices;
            int columns;
            int rows;
        };
        QVector<Sized> floors;
        float plant_width = 0.0f;
        for (auto floor = plant->begin(); floor != plant->end(); ++floor) {
            auto devices = floor.value();
            std::sort(devices.begin(), devices.end(), [](const auto& a, const auto& b) {
                return a.device_id != b.device_id ? a.device_id < b.device_id : a.sensor < b.sensor;
            });
            const int n = devices.size();
            const int columns =
                std::clamp(static_cast<int>(std::ceil(std::sqrt(n * 1.6))), 1, max_columns);
            const int rows = (n + columns - 1) / columns;
            floors.push_back({floor.key(), devices, columns, rows});
            plant_width = std::max(plant_width, columns * spacing);
        }

        float zone_z = 0.0f;
        for (const auto& f : floors) {
            PlantZone zone;
            zone.plant = plant.key();
            zone.floor = f.floor;
            zone.width = plant_width;
            zone.depth = f.rows * spacing;
            zone.x = plant_x + plant_width / 2;
            zone.z = zone_z + zone.depth / 2;
            zone.count = f.devices.size();
            const int zone_index = scene.zones.size();

            const float grid_w = f.columns * spacing;
            for (int i = 0; i < f.devices.size(); ++i) {
                const auto& d = f.devices[i];
                PlacedDevice p;
                p.device_id = d.device_id;
                p.sensor = d.sensor;
                p.plant = d.plant;
                p.floor = d.floor;
                p.value = d.value;
                p.critical = d.critical;
                p.zone = zone_index;
                p.x = zone.x - grid_w / 2 + (i % f.columns + 0.5f) * spacing;
                p.z = zone_z + (i / f.columns + 0.5f) * spacing;
                p.height = normalise(d.sensor, d.value);
                if (d.critical)
                    ++zone.critical;
                scene.devices.push_back(p);
            }
            scene.zones.push_back(zone);
            zone_z += zone.depth + zone_gap;
        }
        plant_x += plant_width + plant_gap;
    }

    // Centre on the origin.
    float min_x = 1e30f, max_x = -1e30f, min_z = 1e30f, max_z = -1e30f;
    for (const auto& z : scene.zones) {
        min_x = std::min(min_x, z.x - z.width / 2);
        max_x = std::max(max_x, z.x + z.width / 2);
        min_z = std::min(min_z, z.z - z.depth / 2);
        max_z = std::max(max_z, z.z + z.depth / 2);
    }
    const float cx = (min_x + max_x) / 2, cz = (min_z + max_z) / 2;
    for (auto& z : scene.zones) {
        z.x -= cx;
        z.z -= cz;
    }
    for (auto& d : scene.devices) {
        d.x -= cx;
        d.z -= cz;
    }
    scene.width = max_x - min_x;
    scene.depth = max_z - min_z;
    return scene;
}

int PlantLayout::nearest(const PlantScene& scene, float x, float z, float max_dist) {
    int best = -1;
    float best_d2 = max_dist * max_dist;
    for (int i = 0; i < scene.devices.size(); ++i) {
        const float dx = scene.devices[i].x - x;
        const float dz = scene.devices[i].z - z;
        const float d2 = dx * dx + dz * dz;
        if (d2 <= best_d2) {
            best_d2 = d2;
            best = i;
        }
    }
    return best;
}

} // namespace ui
