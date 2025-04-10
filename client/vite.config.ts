import { defineConfig } from "vite";
import vue from "@vitejs/plugin-vue";

export default defineConfig({
  plugins: [
    vue({
      template: {
        compilerOptions: {
          // 所有以 mdui- 开头的标签名都是 mdui 组件
          isCustomElement: (tag) => tag.startsWith("mdui-"),
        },
      },
    }),
  ],
  server: {
    proxy: {
      "/api": {
        // 可以直接连产品来开发前端
        target: "http://192.168.4.1",
        changeOrigin: true,
      },
    },
  },
});
