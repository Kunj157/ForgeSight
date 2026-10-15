export const REPO = "https://github.com/Kunj157/ForgeSight";
export const RELEASES = `${REPO}/releases/latest`;
export const APPIMAGE = `${REPO}/releases/latest/download/ForgeSight-x86_64.AppImage`;
export const IMAGE = "ghcr.io/kunj157/forgesight-backend:latest";
export const media = (name: string) => `${import.meta.env.BASE_URL}media/${name}`;
