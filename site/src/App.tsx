import { Architecture } from "./components/architecture";
import { Download, Footer } from "./components/download";
import { Features } from "./components/features";
import { Hero } from "./components/hero";
import { Nav } from "./components/nav";
import { Performance } from "./components/performance";
import { Showcase } from "./components/showcase";

export default function App() {
  return (
    <div id="top" className="min-h-screen overflow-x-hidden">
      <Nav />
      <main>
        <Hero />
        <Showcase />
        <Features />
        <Architecture />
        <Performance />
        <Download />
      </main>
      <Footer />
    </div>
  );
}
