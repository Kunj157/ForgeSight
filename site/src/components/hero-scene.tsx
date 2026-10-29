import { Canvas, useFrame, useThree } from "@react-three/fiber";
import { useMemo, useRef } from "react";
import * as THREE from "three";

// The in-app 3D plant floor, echoed for the web: zones of instanced device
// pillars whose heights breathe like live readings, a few critical ones
// pulsing red with expanding ground rings.

const ZONES = [
  { x: -9, z: -5, cols: 8, rows: 5 },
  { x: 9, z: -5, cols: 8, rows: 5 },
  { x: -9, z: 6.5, cols: 8, rows: 4 },
  { x: 9, z: 6.5, cols: 8, rows: 4 },
];
const SPACING = 1.6;

type Device = { x: number; z: number; phase: number; base: number; critical: boolean };

function mulberry32(seed: number) {
  return () => {
    seed |= 0;
    seed = (seed + 0x6d2b79f5) | 0;
    let r = Math.imul(seed ^ (seed >>> 15), 1 | seed);
    r = (r + Math.imul(r ^ (r >>> 7), 61 | r)) ^ r;
    return ((r ^ (r >>> 14)) >>> 0) / 4294967296;
  };
}

function useDevices(): Device[] {
  return useMemo(() => {
    const rnd = mulberry32(11);
    const out: Device[] = [];
    for (const zone of ZONES) {
      for (let r = 0; r < zone.rows; r++) {
        for (let c = 0; c < zone.cols; c++) {
          out.push({
            x: zone.x + (c - (zone.cols - 1) / 2) * SPACING,
            z: zone.z + (r - (zone.rows - 1) / 2) * SPACING,
            phase: rnd() * Math.PI * 2,
            base: 0.6 + rnd() * 2.6,
            critical: rnd() < 0.06,
          });
        }
      }
    }
    return out;
  }, []);
}

const LOW = new THREE.Color("#2a3a5a");
const HIGH = new THREE.Color("#5b8cff");
const CRIT = new THREE.Color("#ff5d55");

function Pillars({ reduced }: { reduced: boolean }) {
  const devices = useDevices();
  const mesh = useRef<THREE.InstancedMesh>(null);
  const helper = useMemo(() => new THREE.Object3D(), []);
  const color = useMemo(() => new THREE.Color(), []);
  const rise = useRef(reduced ? 1 : 0);

  useFrame(({ clock }, dt) => {
    const m = mesh.current;
    if (!m) return;
    rise.current = Math.min(1, rise.current + dt * 0.7);
    const ease = 1 - Math.pow(1 - rise.current, 3);
    const t = reduced ? 0 : clock.getElapsedTime();
    devices.forEach((d, i) => {
      const h = Math.max(0.08, (d.base + Math.sin(t * 0.9 + d.phase) * 0.5 + (d.critical ? 1.6 : 0)) * ease);
      helper.position.set(d.x, h / 2, d.z);
      helper.scale.set(0.9, h, 0.9);
      helper.updateMatrix();
      m.setMatrixAt(i, helper.matrix);
      if (d.critical) {
        color.copy(CRIT).multiplyScalar(0.85 + 0.25 * Math.sin(t * 5 + d.phase));
      } else {
        color.copy(LOW).lerp(HIGH, Math.min(1, h / 3.6));
      }
      m.setColorAt(i, color);
    });
    m.instanceMatrix.needsUpdate = true;
    if (m.instanceColor) m.instanceColor.needsUpdate = true;
  });

  return (
    <instancedMesh ref={mesh} args={[undefined, undefined, devices.length]} castShadow receiveShadow>
      <boxGeometry args={[1, 1, 1]} />
      <meshStandardMaterial metalness={0.25} roughness={0.35} />
    </instancedMesh>
  );
}

function Rings({ reduced }: { reduced: boolean }) {
  const devices = useDevices().filter((d) => d.critical);
  const mesh = useRef<THREE.InstancedMesh>(null);
  const helper = useMemo(() => new THREE.Object3D(), []);
  useFrame(({ clock }) => {
    const m = mesh.current;
    if (!m) return;
    const t = reduced ? 0.4 : clock.getElapsedTime();
    devices.forEach((d, i) => {
      const p = (t * 0.6 + d.phase) % 1;
      helper.position.set(d.x, 0.06, d.z);
      helper.rotation.set(-Math.PI / 2, 0, 0);
      helper.scale.setScalar(0.6 + p * 2.4);
      helper.updateMatrix();
      m.setMatrixAt(i, helper.matrix);
    });
    m.instanceMatrix.needsUpdate = true;
    (m.material as THREE.MeshBasicMaterial).opacity = 0.55;
  });
  return (
    <instancedMesh ref={mesh} args={[undefined, undefined, Math.max(1, devices.length)]}>
      <ringGeometry args={[0.85, 1, 48]} />
      <meshBasicMaterial color="#ff5d55" transparent opacity={0.5} toneMapped={false} depthWrite={false} />
    </instancedMesh>
  );
}

function Floor() {
  return (
    <group>
      <mesh rotation={[-Math.PI / 2, 0, 0]} receiveShadow>
        <planeGeometry args={[90, 70]} />
        <meshStandardMaterial color="#0b1019" roughness={0.95} />
      </mesh>
      <gridHelper args={[90, 56, "#1d2638", "#141b28"]} position={[0, 0.01, 0]} />
      {ZONES.map((z, i) => (
        <mesh key={i} position={[z.x, 0.04, z.z]} receiveShadow>
          <boxGeometry args={[z.cols * SPACING + 0.8, 0.08, z.rows * SPACING + 0.8]} />
          <meshStandardMaterial color={i === 1 ? "#24141a" : "#131a27"} roughness={0.7} />
        </mesh>
      ))}
    </group>
  );
}

/** Slow orbit with a gentle parallax toward the pointer. */
function Rig({ reduced }: { reduced: boolean }) {
  const { camera, pointer } = useThree();
  const target = useMemo(() => new THREE.Vector3(), []);
  useFrame(({ clock }) => {
    const t = reduced ? 0.6 : clock.getElapsedTime() * 0.06 + 0.6;
    const r = 40;
    target.set(Math.sin(t) * r + pointer.x * 3, 21 + pointer.y * 2, Math.cos(t) * r);
    camera.position.lerp(target, 0.05);
    camera.lookAt(0, 0, 0.5);
  });
  return null;
}

export default function HeroScene({ reduced }: { reduced: boolean }) {
  return (
    <Canvas
      shadows
      camera={{ position: [24, 30, 30], fov: 36 }}
      dpr={[1, 1.75]}
      frameloop={reduced ? "demand" : "always"}
      gl={{ antialias: true, alpha: true, powerPreference: "high-performance" }}
      aria-hidden
    >
      <fog attach="fog" args={["#06080c", 40, 85]} />
      <ambientLight intensity={0.45} />
      <directionalLight
        position={[12, 24, 10]}
        intensity={1.6}
        castShadow
        shadow-mapSize={[2048, 2048]}
        shadow-camera-left={-30}
        shadow-camera-right={30}
        shadow-camera-top={30}
        shadow-camera-bottom={-30}
      />
      <pointLight position={[-18, 8, -12]} intensity={120} distance={60} color="#9a7bff" />
      <pointLight position={[18, 6, 14]} intensity={90} distance={50} color="#5b8cff" />
      <Rig reduced={reduced} />
      <Floor />
      <Pillars reduced={reduced} />
      <Rings reduced={reduced} />
    </Canvas>
  );
}
