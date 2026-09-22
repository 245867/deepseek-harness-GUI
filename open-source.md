# 开源发布清单

发布以下内容：C++ 宿主、`web` 前端资源、`models.yaml`、`secrets.env.example`、`settings.example.yaml`，以及本文档。

**不要发布**：`secrets.env`、私有的 `settings.yaml`、日志、会话数据，以及任何包含本机配置的编译产物。

壁纸预设转自分发于 MIT 许可的 `Fei-Away/Codex-Dream-Skin` 仓库。发布时请把图片与其来源说明 `jade-frontend/web/wallpapers/SOURCES.md` 一起保留。

推荐的忽略规则：

```gitignore
secrets.env
settings.yaml
*.log
sessions/
storages/
jade-frontend/DSHStudio.exe
```

换一台新机器时，把 `secrets.env.example` 复制为 `secrets.env`、把 `settings.example.yaml` 复制为 `settings.yaml`，替换其中的占位提供方地址、填入提供方密钥，再带着这些本地文件启动 dsh 即可。前端只会从本地 dsh 接口接收模型目录数据和会话事件，不会接触到任何密钥。
