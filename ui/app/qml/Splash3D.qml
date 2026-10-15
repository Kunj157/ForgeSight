import QtQuick
import QtQuick3D

// Connecting screen: the ForgeSight "layers" mark as three floating slabs
// that bob and slowly turn, lit by an accent rim light. Shown only while
// the app has no data yet; Main falls back to a 2D version without 3D.
Item {
    id: root

    property real t: 0
    NumberAnimation on t {
        from: 0; to: Math.PI * 2
        duration: 6000
        loops: Animation.Infinite
        running: root.visible && !Theme.reducedMotion
    }

    View3D {
        anchors.fill: parent
        renderMode: View3D.Offscreen
        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Transparent
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
        }

        PerspectiveCamera { z: 380; y: 60; eulerRotation.x: -9; fieldOfView: 30 }
        DirectionalLight { eulerRotation.x: -40; eulerRotation.y: -30; brightness: 1.1 }
        PointLight { position: Qt.vector3d(-160, 40, 120); color: Theme.accent2; brightness: 3; quadraticFade: 0.0004 }
        PointLight { position: Qt.vector3d(160, -20, 120); color: Theme.accent; brightness: 3; quadraticFade: 0.0004 }

        Node {
            eulerRotation.y: root.t * 180 / Math.PI * 0.5 + 30
            eulerRotation.x: 22

            Repeater3D {
                model: 3
                delegate: Model {
                    source: "#Cube"
                    y: 34 - index * 34 + Math.sin(root.t + index * 0.9) * 7
                    scale: Qt.vector3d(1.1 - index * 0.12, 0.13, 1.1 - index * 0.12)
                    eulerRotation.y: 45
                    materials: PrincipledMaterial {
                        baseColor: index === 0 ? Theme.accent : index === 1 ? Qt.tint(Theme.accent, Theme.alpha(Theme.accent2, 0.5)) : Theme.accent2
                        metalness: 0.55
                        roughness: 0.22
                        emissiveFactor: Qt.vector3d(0.06, 0.07, 0.14)
                    }
                }
            }
        }
    }
}
