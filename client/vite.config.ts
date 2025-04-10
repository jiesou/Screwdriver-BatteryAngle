import { defineConfig } from "vite";
import vue from "@vitejs/plugin-vue";

// https://vite.dev/config/
export default defineConfig({
  plugins: [vue({
    template: {
      compilerOptions: {
        // 所有以 mdui- 开头的标签名都是 mdui 组件
        isCustomElement: (tag) => tag.startsWith("mdui-"),
      },
    },
  })],
});
