(function () {
  'use strict'
  const API = 'http://127.0.0.1:3080'
  const state = { sessions: [], current: null, history: [], models: null, modelConfig: null, pluginInventory: null, pluginFilter: 'all', pluginQuery: '', pluginUpdating: new Set(), connected: false, connectionRetry: null, busy: false, pendingSince: null, filter: '', archivedIds: new Set(), deletedIds: new Set(), sessionView: 'active', permissionChanging: false, modelChanging: false }
  const $ = id => document.getElementById(id)
  const text = value => typeof value === 'string' ? value : JSON.stringify(value, null, 2)
  const messageText = data => {
    const source = data && data.message ? data.message : data
    if (!source || !Array.isArray(source.content)) return ''
    return source.content.filter(part => part && part.type === 'text').map(part => part.text || '').join('')
  }
  function setConnection(ok) { state.connected = ok; $('connectionDot').className = 'dot ' + (ok ? 'online' : 'offline'); $('connectionText').textContent = ok ? '已连接' : '离线' }
  function toast(value) { const el = $('toast'); el.textContent = value; el.classList.add('show'); setTimeout(() => el.classList.remove('show'), 2400) }
  function parseIpc(value) {
    let current = value
    for (let i = 0; i < 3 && typeof current === 'string'; i += 1) {
      try { current = JSON.parse(current) } catch (_) { break }
    }
    return current
  }
  async function nativeInvoke(channel, payload) {
    if (!window.jade || typeof window.jade.invoke !== 'function') throw new Error('JadeView 原生桥接不可用')
    try {
      return parseIpc(await window.jade.invoke(channel, JSON.stringify(payload || {}), { timeout: 30000 }))
    } catch (_) {
      return parseIpc(await window.jade.invoke(channel, payload || {}, { timeout: 30000 }))
    }
  }
  async function rpc(method, payload) {
    const envelope = await nativeInvoke('api_request', { method, payload })
    if (!envelope.result || !envelope.result.ok) throw new Error(envelope.result?.error?.message || '请求失败')
    setConnection(true); return envelope.result.value
  }
  function titleFor(item) { return item.projections?.values?.title || (item.blank ? '新会话' : '未命名会话') }
  function timeFor(ms) { const date = new Date(ms); const today = new Date(); if (date.toDateString() === today.toDateString()) return date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }); return date.toLocaleDateString([], { month: 'short', day: 'numeric' }) }
  function hideContextMenu() { $('contextMenu').classList.add('hidden') }
  function placeContextMenu(event, actions) { const menu = $('contextMenu'); menu.replaceChildren(); actions.forEach(action => { const button = document.createElement('button'); button.type = 'button'; button.className = action.danger ? 'danger' : ''; button.innerHTML = `<span class="menu-icon">${action.icon}</span><span>${action.label}</span>`; button.addEventListener('click', () => { hideContextMenu(); action.run() }); menu.appendChild(button) }); menu.classList.remove('hidden'); const width = menu.offsetWidth; const height = menu.offsetHeight; menu.style.left = Math.max(8, Math.min(window.innerWidth - width - 8, event.clientX)) + 'px'; menu.style.top = Math.max(8, Math.min(window.innerHeight - height - 8, event.clientY)) + 'px' }
  function showConfirmation(title, description, confirmLabel, action) { const dialog = $('confirmDialog'); $('confirmTitle').textContent = title; $('confirmDescription').textContent = description; $('confirmAccept').textContent = confirmLabel; const close = () => { dialog.classList.add('hidden'); $('confirmAccept').onclick = null; $('confirmCancel').onclick = null }; $('confirmCancel').onclick = close; $('confirmAccept').onclick = async () => { close(); await action() }; dialog.classList.remove('hidden'); $('confirmAccept').focus() }
  function copyText(content, success = '已复制') { if (!content) return; const done = () => toast(success); if (navigator.clipboard?.writeText) { navigator.clipboard.writeText(content).then(done).catch(() => fallbackCopy()) } else fallbackCopy(); function fallbackCopy() { const textarea = document.createElement('textarea'); textarea.value = content; textarea.style.position = 'fixed'; textarea.style.opacity = '0'; document.body.appendChild(textarea); textarea.select(); const copied = document.execCommand('copy'); textarea.remove(); copied ? done() : toast('复制失败，请检查系统剪贴板权限') } }
  function copyMessage(content) { copyText(content, '已复制消息') }
  function confirmDeleteSession(item) { const name = titleFor(item); showConfirmation('彻底删除会话？', `“${name}” 的全部本地记录将被永久删除。`, '继续', () => showConfirmation('再次确认删除？', '此操作无法恢复，确认彻底删除该会话？', '彻底删除', async () => { try { const result = await nativeInvoke('session_context_action', { action: 'delete', sessionId: item.sessionId }); if (!result?.ok) throw new Error(result?.error || '删除会话失败'); state.deletedIds.add(item.sessionId); state.archivedIds.delete(item.sessionId); state.sessions = state.sessions.filter(session => session.sessionId !== item.sessionId); if (state.current === item.sessionId) { state.current = null; state.history = []; $('sessionTitle').textContent = '准备开始'; $('messages').replaceChildren() } renderSessions(); refreshSessions(); setTimeout(refreshSessions, 1200); toast('会话已彻底删除') } catch (error) { toast(error.message) } })) }
  async function setSessionArchived(item, archived) { try { const result = await nativeInvoke('session_archive', { action: 'set', sessionId: item.sessionId, archived }); if (!result?.ok) throw new Error(result?.error || '无法更新归档状态'); archived ? state.archivedIds.add(item.sessionId) : state.archivedIds.delete(item.sessionId); state.sessionView = archived ? 'archived' : 'active'; renderSessions(); toast(archived ? '会话已归档' : '会话已恢复到列表') } catch (error) { toast(error.message) } }
  function showSessionContextMenu(event, item) { event.preventDefault(); event.stopPropagation(); const archived = state.archivedIds.has(item.sessionId); placeContextMenu(event, [{ icon: '□', label: '打开文件夹', run: async () => { try { const result = await nativeInvoke('session_context_action', { action: 'open_folder', sessionId: item.sessionId, cwd: item.cwd || '' }); if (!result?.ok) throw new Error(result?.error || '无法打开工作区文件夹') } catch (error) { toast(error.message) } } }, { icon: '⧉', label: '复制会话 ID', run: () => copyText(item.sessionId, '已复制会话 ID') }, { icon: archived ? '↩' : '⌑', label: archived ? '恢复到会话列表' : '归档', run: () => setSessionArchived(item, !archived) }, { icon: '×', label: '彻底删除', danger: true, run: () => confirmDeleteSession(item) }]) }
  function renderSessions() {
    const list = $('sessions'); const active = state.sessions.filter(item => !state.archivedIds.has(item.sessionId)); const archived = state.sessions.filter(item => state.archivedIds.has(item.sessionId)); const filtered = (state.sessionView === 'archived' ? archived : active).filter(s => titleFor(s).toLowerCase().includes(state.filter.toLowerCase()))
    $('sessionCount').textContent = active.length; $('archiveCount').textContent = archived.length; $('activeSessions').classList.toggle('active', state.sessionView === 'active'); $('archivedSessions').classList.toggle('active', state.sessionView === 'archived')
    list.replaceChildren()
    if (!filtered.length) { const empty = document.createElement('div'); empty.className = 'session-meta'; empty.style.padding = '20px 10px'; empty.textContent = state.sessionView === 'archived' ? '归档中没有会话' : '没有匹配的会话'; list.appendChild(empty); return }
    filtered.forEach(item => { const row = document.createElement('button'); row.className = 'session' + (state.current === item.sessionId ? ' active' : ''); row.dataset.id = item.sessionId
      const mark = document.createElement('span'); mark.className = 'session-mark'; mark.textContent = '✦'; const copy = document.createElement('span'); copy.className = 'session-copy'; const title = document.createElement('span'); title.className = 'session-title'; title.textContent = titleFor(item); const meta = document.createElement('span'); meta.className = 'session-meta'; meta.textContent = item.cwd ? item.cwd.split(/[\\/]/).pop() + ' · ' + timeFor(item.updatedAt) : timeFor(item.updatedAt); copy.append(title, meta); const stateDot = document.createElement('span'); stateDot.className = 'session-state' + (item.running ? ' live' : ''); row.append(mark, copy, stateDot); row.addEventListener('click', () => openSession(item.sessionId)); row.addEventListener('contextmenu', event => showSessionContextMenu(event, item)); list.appendChild(row) })
  }
  function addMessage(role, content, pending) { const wrap = document.createElement('div'); wrap.className = 'message ' + role + (pending ? ' pending' : ''); const avatar = document.createElement('div'); avatar.className = 'avatar'; avatar.textContent = role === 'user' ? '你' : 'D'; const body = document.createElement('div'); const bubble = document.createElement('div'); bubble.className = 'bubble'; bubble.textContent = content; bubble.addEventListener('contextmenu', event => { event.preventDefault(); event.stopPropagation(); placeContextMenu(event, [{ icon: '⧉', label: '复制', run: () => copyMessage(bubble.textContent || '') }]) }); body.appendChild(bubble); if (!pending) { const meta = document.createElement('div'); meta.className = 'message-meta'; meta.textContent = role === 'user' ? '刚刚' : (state.models?.current?.model || 'DSH'); body.appendChild(meta) } wrap.append(avatar, body); $('messages').appendChild(wrap); $('messages').scrollTop = $('messages').scrollHeight; return bubble }
  function setComposerBusy(busy, status) { state.busy = busy; const button = $('sendButton'); button.classList.toggle('queue-mode', busy); button.querySelector('span').textContent = busy ? '加入队列' : '发送'; button.querySelector('b').textContent = busy ? '＋' : '↗'; if (status) $('statusText').textContent = status }
  function copyConversation() { const content = [...$('messages').children].map(node => { if (node.classList.contains('message')) return `${node.classList.contains('user') ? '用户' : 'DSH'}：${node.querySelector('.bubble')?.textContent || ''}`; if (node.classList.contains('execution-card')) return `执行过程：\n${[...node.querySelectorAll('.execution-item')].map(item => `- ${item.textContent}`).join('\n')}`; return '' }).filter(Boolean).join('\n\n'); copyText(content, '已复制全部消息') }
  function hasActiveTurn(events) { const active = new Set(); events.forEach(entry => { const data = entry.event?.data || {}; if (entry.event?.type === 'turn/start') active.add(data.turn); if (entry.event?.type === 'turn/end') active.delete(data.turn) }); return active.size > 0 }
  function executionText(entry) {
    const type = entry.event?.type || ''; const data = entry.event?.data || {}; const chunk = data.chunk || {}; const tool = data.toolName || data.tool?.name || data.call?.name || data.name || '工具'
    if (type === 'step/start') return `开始第 ${data.step || 1} 步：分析任务并规划操作`
    if (type === 'step/end') return `第 ${data.step || 1} 步完成`
    if (type === 'request/header') return '已准备模型请求与可用工具'
    if (type === 'agent/inbox/spliced') return '已接收消息，等待调度执行'
    if (type === 'llm/retry') return `模型请求遇到问题，准备第 ${data.retry || 1} 次重试`
    if (type === 'llm/retry-started') return `正在进行第 ${data.retry || 1} 次模型重试`
    if (type === 'approval/asked') return '等待你确认 PowerShell 操作后继续执行'
    if (type === 'assistant/chunk' && chunk.type === 'block-start' && chunk.blockType === 'reasoning') return '正在分析与规划下一步'
    if (type === 'assistant/chunk' && chunk.type === 'block-start' && /tool/i.test(chunk.blockType || '')) return '正在准备工具调用'
    if (/tool/.test(type)) return /error|fail/.test(type) ? `工具执行失败：${tool}` : /end|result|complete/.test(type) ? `工具执行完成：${tool}` : `正在调度工具：${tool}`
    if (/subagent/.test(type)) return '正在调度子代理任务'
    if (/workflow/.test(type)) return '正在执行工作流步骤'
    if (/goal/.test(type)) return '正在更新目标执行进度'
    if (/plan/.test(type)) return '正在更新执行计划'
    return ''
  }
  function addExecutionCard(turn) {
    const card = document.createElement('section'); card.className = 'execution-card running'; card.dataset.turn = String(turn)
    const head = document.createElement('div'); head.className = 'execution-head'
    const title = document.createElement('strong'); title.textContent = '执行过程'
    const status = document.createElement('div'); status.className = 'execution-status'; const indicator = document.createElement('span'); indicator.className = 'execution-indicator'; const stateText = document.createElement('span'); stateText.className = 'execution-state'; stateText.textContent = '正在执行'; status.append(indicator, stateText); head.append(title, status)
    const list = document.createElement('div'); list.className = 'execution-list'
    const foot = document.createElement('div'); foot.className = 'execution-foot'
    const approval = document.createElement('div'); approval.className = 'execution-approval hidden'
    const copy = document.createElement('button'); copy.className = 'execution-copy'; copy.type = 'button'; copy.textContent = '复制全部消息'; copy.title = '复制当前会话的消息与执行过程'; copy.addEventListener('click', copyConversation)
    foot.append(approval, copy); card.append(head, list, foot); $('messages').appendChild(card)
    return { card, stateText, list, approval, seen: new Set(), activeItem: null }
  }
  function addExecutionItem(card, text, waitingApproval = false) { if (!text || card.seen.has(text)) return; card.seen.add(text); if (card.activeItem) card.activeItem.classList.remove('active'); const item = document.createElement('div'); item.className = 'execution-item active'; item.textContent = text; card.list.appendChild(item); card.activeItem = item; if (waitingApproval) { card.card.classList.add('awaiting-approval'); card.stateText.textContent = '等待确认' } else if (card.card.classList.contains('running')) { card.card.classList.remove('awaiting-approval'); card.stateText.textContent = '正在执行' } }
  async function answerApproval(card, approval, outcome) { const controls = card.approval.querySelectorAll('button'); controls.forEach(button => { button.disabled = true }); card.stateText.textContent = '正在提交确认'; try { const result = await nativeInvoke('approval_action', { sessionId: state.current, approvalId: approval.id, outcome }); if (!result?.ok) throw new Error(result?.error || '授权请求已失效'); card.approval.classList.add('hidden'); toast(outcome === 'allowed-once' ? '已允许本次操作' : '已拒绝本次操作'); setTimeout(() => { refreshSessions(); if (state.current) openSession(state.current) }, 300) } catch (error) { controls.forEach(button => { button.disabled = false }); card.stateText.textContent = '等待确认'; toast(error.message) } }
  function showApprovalActions(card, approval) { card.approval.replaceChildren(); const details = document.createElement('span'); details.className = 'approval-reason'; details.textContent = approval.reason || `工具 ${approval.toolName || '操作'} 请求一次性授权`; const reject = document.createElement('button'); reject.type = 'button'; reject.className = 'approval-reject'; reject.textContent = '拒绝'; reject.addEventListener('click', () => answerApproval(card, approval, 'rejected')); const allow = document.createElement('button'); allow.type = 'button'; allow.className = 'approval-allow'; allow.textContent = '允许一次'; allow.addEventListener('click', () => answerApproval(card, approval, 'allowed-once')); card.approval.append(details, reject, allow); card.approval.classList.remove('hidden') }
  function addExecutionEvent(card, entry) { const type = entry.event?.type || ''; const data = entry.event?.data || {}; if (type === 'approval/asked') showApprovalActions(card, data); if (type === 'approval/decided') card.approval.classList.add('hidden'); addExecutionItem(card, executionText(entry), type === 'approval/asked') }
  function finishExecutionCard(card, reason) { card.card.classList.remove('running', 'awaiting-approval'); card.approval.classList.add('hidden'); const failure = reason?.kind === 'error'; card.card.classList.toggle('failed', failure); card.stateText.textContent = failure ? '已失败' : reason?.kind === 'interrupted' ? '已中断' : '已完成'; if (failure) addExecutionItem(card, `执行失败：${reason.error?.message || '未知错误'}`) }
  function renderHistory() { const container = $('messages'); container.replaceChildren(); const messages = state.history.filter(entry => entry.event?.type === 'assistant/message' || (entry.event?.type === 'user/message' && entry.event?.data?.source?.kind === 'user')); if (!messages.length) { const empty = document.createElement('div'); empty.className = 'empty'; empty.innerHTML = '<div class="empty-orb">✦</div><h2>你好，我是 DSH</h2><p>选择一个模型，开始你的下一次探索。</p><div class="quick-prompts"><button data-prompt="帮我分析这个项目的结构">分析项目结构</button><button data-prompt="写一个简洁的实现方案">设计实现方案</button><button data-prompt="解释这段代码的作用">解释代码</button></div>'; container.appendChild(empty); bindQuickPrompts(); return }
    const cards = new Map(); let latestCard = null
    state.history.forEach(entry => { const type = entry.event?.type; const data = entry.event?.data || {}; if (type === 'step/start') { let card = cards.get(data.turn); if (!card) { card = addExecutionCard(data.turn); cards.set(data.turn, card) } latestCard = card; addExecutionEvent(card, entry); return } if (type === 'turn/end') { const card = cards.get(data.turn) || latestCard; if (card) finishExecutionCard(card, data.reason); return } if (type === 'user/message' && data.source?.kind === 'user') { const content = messageText(data); if (content.trim()) { addMessage('user', content, false); const card = cards.get(data.turn) || latestCard; if (card) $('messages').appendChild(card.card) } return } if (type === 'assistant/message') { const content = messageText(data); if (content.trim()) addMessage('assistant', content, false); return } const card = cards.get(data.turn) || latestCard; if (card) addExecutionEvent(card, entry) })
    container.scrollTop = container.scrollHeight
  }
  async function openSession(id) { state.current = id; const current = state.sessions.find(s => s.sessionId === id); $('sessionTitle').textContent = current ? titleFor(current) : '当前会话'; renderSessions(); try { const result = await rpc('session.history', { sessionId: id, maxMessages: 200 }); state.history = result.events || []; renderHistory(); setComposerBusy(hasActiveTurn(state.history) || Boolean(current?.running), current?.running ? '正在执行…' : '就绪'); const models = await rpc('session.models', { sessionId: id }); state.models = models; renderModel(); } catch (error) { setConnection(false); $('currentModel').textContent = '连接失败'; $('composerModelLabel').textContent = '连接失败'; toast(error.message) } }
  async function refreshSessions() { try { const result = await rpc('session.list', {}); if (state.connectionRetry) { clearTimeout(state.connectionRetry); state.connectionRetry = null } state.sessions = (result.items || []).filter(item => !state.deletedIds.has(item.sessionId)); renderSessions(); const visible = state.sessions.filter(item => state.sessionView === 'archived' ? state.archivedIds.has(item.sessionId) : !state.archivedIds.has(item.sessionId)); if (!state.current && visible.length) await openSession(visible[0].sessionId); else if (state.current && state.sessions.some(s => s.sessionId === state.current)) await openSession(state.current); } catch (error) { setConnection(false); $('statusText').textContent = '等待后台服务…'; if (!state.connectionRetry) state.connectionRetry = setTimeout(() => { state.connectionRetry = null; refreshSessions() }, 1500) } }
  async function newSession() { try { const result = await rpc('session.create', { cwd: '.' }); state.current = result.sessionId; await refreshSessions(); toast('已创建新会话') } catch (error) { toast(error.message) } }
  function displayGroups() { const live = state.models?.groups || []; const configured = state.modelConfig?.groups || []; if (!configured.length) return live; const used = new Set(); const ordered = configured.map(group => { const available = live.find(item => item.id === group.id); if (!available) return null; used.add(group.id); const models = group.models.map(model => available.models.find(item => item.id === model.id) || model).filter(model => model.id); return { ...available, name: group.name || available.name, models } }).filter(group => group && group.models.length); return ordered.concat(live.filter(group => !used.has(group.id))) }
  const protectedPlugins = new Set(['include', 'include:plugin-inventory', 'include:api-gateway', 'include:cordis-host-runner', 'include:web-startup', 'include:webserver', 'include:web-runtime'])
  function pluginPhase(entry) { if (!entry.enabled) return '已关闭'; if (entry.fiberPhase === 'active') return '运行中'; if (entry.fiberPhase === 'failed') return '启动失败'; return entry.fiberPhase || '等待加载' }
  const pluginDescriptions = [
    [/cordis:include/, '加载并组合 DSH 的插件配置树'],
    [/cordis-plugin-timer|\btimer\b/, '提供定时器与延迟任务能力'],
    [/cordis-plugin-hmr|\bhmr\b/, '监听配置变化并热重载插件'],
    [/directory-picker/, '提供本机目录选择与工作区定位'],
    [/plugin-inventory/, '提供插件清单与运行状态查询'],
    [/api-gateway|apiproxy|api-remotes/, '处理本机客户端与 DSH 服务的 API 通信'],
    [/cordis-host-runner|cordis-client-runner/, '负责加载并运行 Cordis 插件树'],
    [/webserver/, '在本机 3080 端口提供 Web 服务'],
    [/web-startup/, '初始化 DSH Web 应用启动流程'],
    [/web-runtime/, '提供原版 DSH Web 客户端运行时'],
    [/client-hmr/, '为客户端提供界面热更新支持'],
    [/client-modules/, '加载客户端功能模块'],
    [/client-connection/, '维护客户端与本机网关的连接'],
    [/client-runtime/, '提供客户端运行时与状态同步'],
    [/ui-settings-plugin-inventory/, '在原版设置中展示插件运行清单'],
    [/ui-settings-plugins/, '在原版设置中提供插件配置页面'],
    [/ui-settings-models/, '在原版设置中提供模型配置'],
    [/ui-settings-general/, '在原版设置中提供常规选项'],
    [/ui-settings/, '提供原版客户端设置框架'],
    [/ui-theme/, '提供客户端主题与外观切换'],
    [/ui-layout/, '提供客户端页面布局'],
    [/ui-sidebar/, '提供客户端侧边栏导航'],
    [/ui-conversation/, '提供原版聊天会话界面'],
    [/ui-tool/, '展示工具调用与执行结果'],
    [/ui-workflow-run/, '展示工作流运行进度'],
    [/ui-deliverables/, '展示交付文件与结果'],
    [/ui-workspace/, '提供工作区信息与文件视图'],
    [/ui-input-trigger/, '处理客户端输入与快捷触发'],
    [/ui-commands/, '提供客户端命令面板'],
    [/ui-skill/, '展示技能与可用能力'],
    [/ui-subagent/, '展示子代理任务与状态'],
    [/ui-jobs/, '展示后台任务队列'],
    [/ui-goal/, '展示目标执行状态'],
    [/ui-message-feedback/, '提供消息反馈入口'],
    [/ui-model-selection/, '提供客户端模型选择器'],
    [/ui-permission/, '提供权限级别选择界面'],
    [/ui-agent-preset/, '提供代理预设选择界面'],
    [/ui-plan/, '展示计划模式与步骤'],
    [/ui-user-questions/, '处理代理向用户发起的问题'],
    [/ui-trajectory/, '展示会话执行轨迹'],
    [/session-title-first-prompt-llm/, '使用首次提问自动生成会话标题'],
    [/session-title/, '管理会话标题'],
    [/session-persistence/, '将会话内容持久化到本机'],
    [/session-query/, '提供会话检索与查询'],
    [/session-projection/, '维护会话界面所需的状态投影'],
    [/session-telemetry/, '记录会话运行遥测信息'],
    [/session-checkpoint/, '保存会话检查点以支持恢复'],
    [/session-stats/, '统计会话用量与运行信息'],
    [/\bdsh-session\b/, '提供会话创建、消息与历史管理'],
    [/\bdsh-llm\b/, '提供大语言模型调用能力'],
    [/llm-pi-ai/, '适配 OpenAI 兼容模型服务'],
    [/llm-retry/, '在模型请求失败时自动重试'],
    [/agent-default-model/, '设置新会话的默认模型'],
    [/agent-loop/, '驱动代理的推理与工具调用循环'],
    [/\bdsh-agent\b/, '提供代理核心执行能力'],
    [/agent-instructions/, '读取并应用项目级代理指令'],
    [/agent-presets/, '管理代理预设及其组合能力'],
    [/credentials/, '安全读取本机模型服务凭据'],
    [/settings-file/, '从本机配置文件读取 DSH 设置'],
    [/\bdsh-settings\b/, '提供 DSH 设置服务'],
    [/attachment-local/, '处理本机文件附件'],
    [/storage-json|storage-domain|\bdsh-storage\b/, '提供本机会话与应用数据存储'],
    [/subprocess/, '运行受控的本机子进程'],
    [/sandbox-policy/, '根据权限策略限制工具访问范围'],
    [/sandbox-local/, '提供本机沙箱执行环境'],
    [/bash-sandbox|pwsh-sandbox/, '为命令行工具提供受限沙箱'],
    [/user-approval|approval/, '在高风险操作前请求用户确认'],
    [/permission-presets|\bpermission\b/, '提供只读、工作区写入与完全访问权限'],
    [/shell-env/, '读取并整理本机 Shell 环境变量'],
    [/tool-bash/, '让代理执行 Bash 命令'],
    [/tool-pwsh/, '让代理执行 PowerShell 命令'],
    [/tool-fs-search/, '让代理搜索工作区文件内容'],
    [/tool-fs/, '让代理读取和修改工作区文件'],
    [/tool-jobs/, '让代理管理后台任务'],
    [/tool-skill/, '让代理调用已安装技能'],
    [/tool-subagent/, '让代理创建和管理子代理'],
    [/tool-workflow/, '让代理创建和运行工作流'],
    [/tool-web/, '让代理访问网页与网络检索能力'],
    [/tool-goal/, '让代理读取和更新执行目标'],
    [/tool-todo/, '让代理维护任务清单'],
    [/tool-ralph/, '提供循环式自动执行工具'],
    [/tool-str-replace-editor/, '提供基于字符串替换的文件编辑工具'],
    [/\bdsh-tools\b/, '注册 DSH 可用工具集合'],
    [/\bdsh-skill\b/, '发现并加载可用技能'],
    [/skill-filesystem/, '从工作区读取技能文件'],
    [/skill-badge/, '为技能提供状态标识'],
    [/\bdsh-commands\b/, '注册 DSH 斜杠命令'],
    [/command-feedback/, '提供消息反馈相关命令'],
    [/command-goal/, '提供目标管理相关命令'],
    [/command-compact/, '提供会话压缩相关命令'],
    [/\bdsh-goal\b/, '管理可持续执行的目标'],
    [/goal-round-driver/, '驱动目标的连续执行轮次'],
    [/plan-mode/, '提供先规划后执行的工作模式'],
    [/token-meter/, '统计上下文与令牌使用情况'],
    [/compaction-tool-result-pruner/, '清理过长的工具结果以节省上下文'],
    [/compaction-basic/, '压缩较长会话以释放上下文空间'],
    [/\bdsh-subagent\b/, '提供子代理协作与任务分发'],
    [/subagent-spawn/, '在当前进程内启动子代理'],
    [/subagent-fork/, '基于当前会话分叉子代理'],
    [/subagent-report/, '汇总子代理执行报告'],
    [/workflow-worker-thread/, '在线程中运行工作流任务'],
    [/timeout-policy/, '限制工具调用的最长执行时间'],
    [/spill-local|spill-policy/, '将超大中间结果暂存到本机'],
    [/tool-result-pruner/, '裁剪过长工具输出'],
    [/repeat-tool-reminder/, '提醒代理避免重复执行同一工具'],
    [/web-search-deepseek/, '通过 DeepSeek 服务执行网页搜索'],
    [/\bdsh-web\b/, '提供网页访问与内容提取能力'],
    [/system-prompt/, '组装代理的系统提示词'],
    [/message-feedback/, '保存对模型回复的反馈'],
    [/workspace/, '维护当前工作区信息'],
    [/\btypert\b/, '提供类型安全的服务与远程调用协议'],
    [/\bdsh-jobs\b/, '提供本机后台任务执行器'],
    [/\bdsh-persona\b/, '提供代理人设与行为配置'],
  ]
  function pluginDescription(entry) {
    const source = `${entry.moduleName || ''} ${entry.entryId || ''}`.toLowerCase()
    return pluginDescriptions.find(([pattern]) => pattern.test(source))?.[1] || '扩展 DSH 运行时功能的插件组件'
  }
  function renderPluginPage() {
    const list = $('pluginPageList')
    if (!list) return
    const entries = state.pluginInventory?.entries || []
    const enabled = entries.filter(entry => entry.enabled).length
    const active = entries.filter(entry => entry.fiberPhase === 'active').length
    $('pluginEnabledCount').textContent = enabled
    $('pluginDisabledCount').textContent = entries.length - enabled
    $('pluginActiveCount').textContent = active
    $('pluginSummary').textContent = state.pluginInventory ? `共 ${entries.length} 个插件，状态来自当前 DSH 运行时` : '正在读取插件清单...'
    const query = state.pluginQuery.trim().toLowerCase()
    const visible = entries.filter(entry => {
      const matchesFilter = state.pluginFilter === 'all' || (state.pluginFilter === 'enabled' ? entry.enabled : !entry.enabled)
      return matchesFilter && (!query || `${entry.moduleName} ${entry.entryId}`.toLowerCase().includes(query))
    })
    list.replaceChildren()
    if (!state.pluginInventory) { const loading = document.createElement('div'); loading.className = 'plugin-page-empty'; loading.textContent = '正在读取插件清单...'; list.appendChild(loading); return }
    if (!visible.length) { const empty = document.createElement('div'); empty.className = 'plugin-page-empty'; empty.textContent = '没有匹配的插件'; list.appendChild(empty); return }
    visible.forEach(entry => {
      const row = document.createElement('div')
      row.className = 'plugin-page-entry' + (entry.enabled ? '' : ' is-disabled')
      const copy = document.createElement('div')
      copy.className = 'plugin-page-copy'
      const name = document.createElement('strong')
      name.className = 'plugin-page-name'
      name.textContent = entry.moduleName || entry.entryId
      const description = document.createElement('span')
      description.className = 'plugin-page-description'
      description.textContent = pluginDescription(entry)
      const meta = document.createElement('div')
      meta.className = 'plugin-page-meta'
      const id = document.createElement('span')
      id.className = 'plugin-page-id'
      id.textContent = entry.entryId
      const status = document.createElement('span')
      status.className = 'plugin-page-status' + (entry.fiberPhase === 'active' ? ' active' : entry.fiberPhase === 'failed' ? ' failed' : '')
      status.textContent = pluginPhase(entry)
      meta.append(id, status)
      copy.append(name, description, meta)
      const toggle = document.createElement('button')
      const protectedPlugin = protectedPlugins.has(entry.entryId)
      toggle.className = 'plugin-switch'
      toggle.type = 'button'
      toggle.setAttribute('role', 'switch')
      toggle.setAttribute('aria-checked', String(Boolean(entry.enabled)))
      toggle.disabled = protectedPlugin || state.pluginUpdating.has(entry.entryId)
      toggle.title = protectedPlugin ? '系统核心插件受保护，不能在此关闭' : (entry.enabled ? '关闭插件' : '开启插件')
      toggle.addEventListener('click', () => setPluginEnabled(entry, !entry.enabled))
      row.append(copy, toggle)
      list.appendChild(row)
    })
  }
  async function setPluginEnabled(entry, enabled) {
    if (protectedPlugins.has(entry.entryId) || state.pluginUpdating.has(entry.entryId)) return
    const action = enabled ? '开启' : '关闭'
    if (!window.confirm(`确认${action}插件？\n${entry.moduleName}\n更改会立即由 DSH 热重载。`)) return
    state.pluginUpdating.add(entry.entryId)
    renderPluginPage()
    try {
      const result = await nativeInvoke('plugin_toggle', { entryId: entry.entryId, enabled })
      if (!result?.ok) throw new Error(result?.error || '插件设置失败')
      entry.enabled = enabled
      entry.fiberPhase = enabled ? 'loading' : null
      toast(`已请求${action} ${entry.moduleName}`)
      renderPluginPage()
      setTimeout(loadPluginInventory, 750)
    } catch (error) {
      toast(error.message)
    } finally {
      state.pluginUpdating.delete(entry.entryId)
      renderPluginPage()
    }
  }
  function renderPluginInventory() {
    const body = document.querySelector('#settingsPanel .drawer-body')
    if (!body) return
    let heading = $('pluginInventoryHeading')
    let list = $('pluginInventory')
    if (!heading) {
      heading = document.createElement('h3')
      heading.id = 'pluginInventoryHeading'
      heading.textContent = '已加载插件'
      list = document.createElement('div')
      list.id = 'pluginInventory'
      list.className = 'plugin-inventory'
      body.append(heading, list)
    }
    list.replaceChildren()
    if (!state.pluginInventory) {
      const status = document.createElement('div')
      status.className = 'plugin-inventory-status'
      status.textContent = '插件清单加载中…'
      list.appendChild(status)
      return
    }
    const entries = state.pluginInventory.entries || []
    const enabled = entries.filter(entry => entry.enabled).length
    heading.textContent = '已加载插件 ' + enabled + '/' + entries.length
    entries.forEach(entry => {
      const row = document.createElement('div')
      row.className = 'plugin-entry' + (entry.enabled ? '' : ' disabled')
      const copy = document.createElement('span')
      copy.className = 'plugin-entry-copy'
      const name = document.createElement('strong')
      name.textContent = entry.moduleName || entry.entryId
      const phase = document.createElement('small')
      phase.textContent = entry.enabled ? (entry.fiberPhase || '已启用') : '已禁用'
      copy.append(name, phase)
      const marker = document.createElement('span')
      marker.className = 'plugin-entry-state ' + (entry.enabled && entry.fiberPhase === 'active' ? 'active' : 'inactive')
      marker.title = entry.enabled ? (entry.fiberPhase || 'enabled') : 'disabled'
      row.append(copy, marker)
      list.appendChild(row)
    })
  }
  async function loadPluginInventory() {
    renderPluginPage()
    try {
      state.pluginInventory = await rpc('pluginInventory/list', { args: {} })
      renderPluginPage()
    } catch (error) {
      state.pluginInventory = { entries: [] }
      renderPluginPage()
      $('pluginSummary').textContent = '无法读取插件清单：' + error.message
    }
  }
  const permissionLabels = { 'read-only': '只读', 'workspace-write': '工作区写入', 'danger-full-access': '完全访问' }
  function currentPermissions() { return state.sessions.find(item => item.sessionId === state.current)?.projections?.values?.permissions }
  function renderPermission() { const picker = $('permissionPicker'); const permissions = currentPermissions(); const current = permissions?.currentValue; picker.disabled = !current || state.permissionChanging; $('permissionLabel').textContent = state.permissionChanging ? '正在切换…' : current ? (permissionLabels[current] || current) : '权限不可用'; picker.title = current ? '切换当前会话权限：' + (permissionLabels[current] || current) : '当前会话没有权限控制'; }
  function renderModel() { const current = state.models?.current; const group = current && displayGroups().find(item => item.id === current.provider); const model = group && group.models.find(item => item.id === current.model); const modelName = model?.name || current?.model || '未选择'; const providerName = group?.name || current?.provider || ''; $('currentModel').textContent = current ? (providerName + ' / ' + modelName) : '未选择'; $('composerModelLabel').textContent = state.modelChanging ? '正在切换…' : modelName; $('composerModelPicker').disabled = !current || state.modelChanging; renderModelGroups(); renderPermission() }
  function renderModelGroups() { const groups = displayGroups(); const target = $('modelGroups'); target.replaceChildren(); groups.forEach(group => { const card = document.createElement('div'); card.className = 'model-group'; const head = document.createElement('div'); head.className = 'model-group-head'; const name = document.createElement('strong'); name.textContent = group.name; const count = document.createElement('small'); count.textContent = group.models.length + ' 个模型'; head.append(name, count); const tags = document.createElement('div'); tags.className = 'model-tags'; group.models.forEach(model => { const tag = document.createElement('span'); tag.className = 'model-tag'; tag.textContent = model.name; tags.appendChild(tag) }); card.append(head, tags); target.appendChild(card) }) }
  function showModelMenu(anchor = $('composerModelPicker')) { const menu = $('modelMenu'); if (!menu.classList.contains('hidden')) { menu.classList.add('hidden'); return } $('permissionMenu').classList.add('hidden'); if (state.modelChanging || !state.models?.current || !anchor) { toast('模型列表还未加载完成'); return } menu.replaceChildren(); displayGroups().forEach(group => { const title = document.createElement('div'); title.className = 'group-title'; title.textContent = group.name; menu.appendChild(title); group.models.forEach(model => { const button = document.createElement('button'); button.type = 'button'; button.className = 'model-option' + (state.models.current.model === model.id ? ' selected' : ''); const copy = document.createElement('span'); const strong = document.createElement('strong'); strong.textContent = model.name; const small = document.createElement('small'); small.textContent = group.name; copy.append(strong, small); const check = document.createElement('span'); check.className = 'check'; check.textContent = state.models.current.model === model.id ? '✓' : ''; button.append(copy, check); button.addEventListener('click', event => { event.stopPropagation(); chooseModel(group.id, model.id) }); menu.appendChild(button) }) }); menu.classList.remove('hidden'); const rect = anchor.getBoundingClientRect(); const opensUpward = anchor.id === 'composerModelPicker'; const top = opensUpward ? Math.max(8, rect.top - menu.offsetHeight - 8) : Math.min(window.innerHeight - menu.offsetHeight - 8, rect.bottom + 8); menu.style.top = top + 'px'; menu.style.left = Math.max(8, Math.min(window.innerWidth - menu.offsetWidth - 8, rect.left)) + 'px' }
  function showPermissionMenu() { const menu = $('permissionMenu'); if (!menu.classList.contains('hidden')) { menu.classList.add('hidden'); return } const permissions = currentPermissions(); if (state.permissionChanging || !permissions?.currentValue) { toast('当前会话没有可用权限级别'); return } $('modelMenu').classList.add('hidden'); menu.replaceChildren(); const title = document.createElement('div'); title.className = 'group-title'; title.textContent = '权限级别'; menu.appendChild(title); permissions.options.filter(option => option.value !== 'custom').forEach(option => { const button = document.createElement('button'); button.type = 'button'; button.className = 'model-option' + (option.value === permissions.currentValue ? ' selected' : ''); const copy = document.createElement('span'); const strong = document.createElement('strong'); strong.textContent = permissionLabels[option.value] || option.name || option.value; const small = document.createElement('small'); small.textContent = option.value === 'read-only' ? '仅查看与分析，不修改文件' : option.value === 'workspace-write' ? '可在当前工作区修改文件' : '完整文件访问，操作前会确认'; copy.append(strong, small); const check = document.createElement('span'); check.className = 'check'; check.textContent = option.value === permissions.currentValue ? '✓' : ''; button.append(copy, check); button.addEventListener('click', event => { event.stopPropagation(); choosePermission(option.value) }); menu.appendChild(button) }); menu.classList.remove('hidden'); const anchor = $('permissionPicker'); const rect = anchor.getBoundingClientRect(); menu.style.top = Math.max(8, rect.top - menu.offsetHeight - 8) + 'px'; menu.style.left = Math.max(8, Math.min(window.innerWidth - menu.offsetWidth - 8, rect.left)) + 'px' }
  function reflectPermission(preset) { state.sessions = state.sessions.map(session => session.sessionId !== state.current ? session : { ...session, projections: { ...session.projections, values: { ...session.projections?.values, permissions: { ...session.projections?.values?.permissions, currentValue: preset } } } }); renderPermission() }
  async function applyPermission(preset) { if (state.permissionChanging || !state.current) return; state.permissionChanging = true; $('permissionMenu').classList.add('hidden'); renderPermission(); try { const result = await rpc('commands/execute', { args: { agentId: state.current, line: '/permission ' + preset } }); if (result?.result?.kind !== 'success') throw new Error(result?.result?.text || 'DSH 未接受权限切换命令'); reflectPermission(preset); toast('权限已切换为' + (permissionLabels[preset] || preset)); setTimeout(() => { refreshSessions() }, 350) } catch (error) { toast(error.message) } finally { state.permissionChanging = false; renderPermission() } }
  function choosePermission(preset) { const permissions = currentPermissions(); if (!state.current || preset === permissions?.currentValue) { $('permissionMenu').classList.add('hidden'); return } $('permissionMenu').classList.add('hidden'); if (preset === 'danger-full-access') { showConfirmation('启用完全访问？', '完全访问允许当前会话执行不受工作区限制的文件操作；高风险操作仍会请求确认。', '启用完全访问', () => applyPermission(preset)); return } void applyPermission(preset) }
  async function chooseModel(provider, model) { if (state.modelChanging || !state.current) return; state.modelChanging = true; $('modelMenu').classList.add('hidden'); renderModel(); try { const result = await rpc('session.selectModel', { sessionId: state.current, provider, model }); state.models.current = result?.selected || { provider, model }; renderModel(); toast('已切换到 ' + (result?.selected?.model || model)); setTimeout(() => { if (state.current) openSession(state.current) }, 250) } catch (error) { toast(error.message) } finally { state.modelChanging = false; renderModel() } }
  function historyMaxSeq(events) { return events.reduce((max, entry) => Math.max(max, Number(entry.event?.seq ?? -1)), -1) }
  async function sendPrompt(value) { const content = value.trim(); if (!content || !state.current) return; const queued = state.busy; state.pendingSince = historyMaxSeq(state.history); setComposerBusy(true, queued ? '已加入队列…' : '正在执行…'); const empty = $('messages .empty'); if (empty) empty.remove(); addMessage('user', content, false); try { await rpc('session.prompt', { sessionId: state.current, mode: 'queue', content: [{ type: 'text', text: content }], clientTimeZone: Intl.DateTimeFormat().resolvedOptions().timeZone }); toast(queued ? '消息已加入队列' : '任务已提交'); setTimeout(refreshSessions, 250) } catch (error) { const pending = [...$('messages').querySelectorAll('.message.user')].pop(); if (pending) pending.classList.add('error'); setComposerBusy(false, '就绪'); toast(error.message) } }
  async function consumeEvents() { const poll = async () => { try { if (state.current) { const result = await rpc('session.history', { sessionId: state.current, maxMessages: 200 }); const next = result.events || []; if (next.length !== state.history.length) { state.history = next; renderHistory(); } const current = state.sessions.find(item => item.sessionId === state.current); const active = hasActiveTurn(next) || Boolean(current?.running); setComposerBusy(active, active ? '正在执行…' : '就绪') } } catch (_) { setConnection(false) } finally { setTimeout(poll, 1400) } }; poll() }
  function handleEvent(event) { if (event.type === 'assistant/message') { const content = messageText(event.data); const pending = [...$('messages').querySelectorAll('.message.pending')].pop(); if (pending) { pending.classList.remove('pending'); pending.querySelector('.bubble').textContent = content } else addMessage('assistant', content, false); state.busy = false; $('statusText').textContent = '就绪'; refreshSessions() } else if (event.type === 'assistant/chunk' && event.data?.chunk?.type === 'text_delta') { const pending = [...$('messages').querySelectorAll('.message.pending')].pop(); if (pending) pending.querySelector('.bubble').textContent = (pending.querySelector('.bubble').textContent === '正在思考…' || pending.querySelector('.bubble').textContent === '等待模型响应…' ? '' : pending.querySelector('.bubble').textContent) + (event.data.chunk.delta || '') } }
  function bindQuickPrompts() { document.querySelectorAll('[data-prompt]').forEach(button => button.addEventListener('click', () => { $('prompt').value = button.dataset.prompt; $('prompt').focus() })) }
  function closeDrawers() { $('settingsPanel').classList.add('hidden'); $('pluginsPanel').classList.add('hidden'); $('aboutPanel').classList.add('hidden') }
  function copyAboutValue(value) { copyText(value, '已复制本地网关地址') }
  function selectedPromptText() { const input = $('prompt'); return input.value.slice(input.selectionStart || 0, input.selectionEnd || 0) }
  function replacePromptSelection(value) { const input = $('prompt'); input.focus(); input.setRangeText(value, input.selectionStart || 0, input.selectionEnd || 0, 'end'); input.dispatchEvent(new Event('input', { bubbles: true })) }
  async function pasteIntoPrompt() { try { let value; try { if (!navigator.clipboard?.readText) throw new Error('clipboard API unavailable'); value = await navigator.clipboard.readText() } catch (_) { const result = await nativeInvoke('clipboard_read', {}); if (!result?.ok) throw new Error(result?.error || '无法读取系统剪贴板'); value = result.text || '' } replacePromptSelection(value) } catch (error) { toast(error.message || '粘贴失败') } }
  function showPromptContextMenu(event) { event.preventDefault(); event.stopPropagation(); const selected = selectedPromptText(); placeContextMenu(event, [{ icon: '⧉', label: '复制', run: () => selected ? copyText(selected, '已复制选中文字') : toast('请先选择文字') }, { icon: '✂', label: '剪切', run: () => { if (!selected) return toast('请先选择文字'); copyText(selected, '已剪切到剪贴板'); replacePromptSelection('') } }, { icon: '▣', label: '粘贴', run: pasteIntoPrompt }, { icon: 'A', label: '全选', run: () => { const input = $('prompt'); input.focus(); input.select() } }]) }
  document.addEventListener('contextmenu', event => { if (!event.target.closest('.bubble') && !event.target.closest('.session')) { event.preventDefault(); hideContextMenu() } }); document.addEventListener('click', event => { if (!event.target.closest('#contextMenu')) hideContextMenu() }); document.addEventListener('keydown', event => { if (event.key === 'Escape') { hideContextMenu(); $('confirmDialog').classList.add('hidden'); closeDrawers() } }); $('newSession').addEventListener('click', newSession); $('clearSession').addEventListener('click', newSession); $('refresh').addEventListener('click', refreshSessions); $('activeSessions').addEventListener('click', () => { state.sessionView = 'active'; renderSessions() }); $('archivedSessions').addEventListener('click', () => { state.sessionView = 'archived'; renderSessions() }); $('search').addEventListener('input', event => { state.filter = event.target.value; renderSessions() }); $('modelPicker').addEventListener('click', event => { event.stopPropagation(); showModelMenu(event.currentTarget) }); $('composerModelPicker').addEventListener('click', event => { event.preventDefault(); event.stopPropagation(); showModelMenu(event.currentTarget) }); document.addEventListener('click', event => { if (!event.target.closest('#modelMenu') && !event.target.closest('#modelPicker') && !event.target.closest('#composerModelPicker')) $('modelMenu').classList.add('hidden') }); $('settings').addEventListener('click', () => { closeDrawers(); $('settingsPanel').classList.remove('hidden') }); $('closeSettings').addEventListener('click', () => $('settingsPanel').classList.add('hidden')); $('about').addEventListener('click', () => { closeDrawers(); $('aboutPanel').classList.remove('hidden') }); $('closeAbout').addEventListener('click', () => $('aboutPanel').classList.add('hidden')); document.querySelectorAll('[data-copy]').forEach(button => button.addEventListener('click', () => copyAboutValue(button.dataset.copy))); $('copyAll').addEventListener('click', copyConversation); $('composer').addEventListener('submit', event => { event.preventDefault(); const input = $('prompt'); sendPrompt(input.value); input.value = ''; input.style.height = 'auto' }); $('prompt').addEventListener('keydown', event => { if (event.key === 'Enter' && !event.shiftKey) { event.preventDefault(); $('composer').requestSubmit() } }); $('prompt').addEventListener('input', event => { event.target.style.height = 'auto'; event.target.style.height = Math.min(event.target.scrollHeight, 140) + 'px' }); bindQuickPrompts(); $('minimize').addEventListener('click', () => nativeInvoke('window_control', { action: 'minimize' })); $('maximize').addEventListener('click', () => nativeInvoke('window_control', { action: 'maximize' })); $('close').addEventListener('click', () => nativeInvoke('window_control', { action: 'close' }));
  $('prompt').addEventListener('contextmenu', showPromptContextMenu)
  $('prompt').addEventListener('keydown', event => { if ((event.ctrlKey || event.metaKey) && !event.altKey && event.key.toLowerCase() === 'v') { event.preventDefault(); pasteIntoPrompt() } })
  $('plugins').addEventListener('click', () => { closeDrawers(); $('pluginsPanel').classList.remove('hidden'); loadPluginInventory() })
  $('closePlugins').addEventListener('click', () => $('pluginsPanel').classList.add('hidden'))
  $('refreshPlugins').addEventListener('click', loadPluginInventory)
  $('pluginSearch').addEventListener('input', event => { state.pluginQuery = event.target.value; renderPluginPage() })
  document.querySelectorAll('[data-plugin-filter]').forEach(button => button.addEventListener('click', () => { state.pluginFilter = button.dataset.pluginFilter; document.querySelectorAll('[data-plugin-filter]').forEach(item => item.classList.toggle('active', item === button)); renderPluginPage() }))
  $('fullWorkspace').addEventListener('click', async () => {
    try {
      const result = await nativeInvoke('open_full_workspace', {})
      if (!result?.ok) throw new Error(result?.error || 'Unable to open DSH workspace')
    } catch (error) {
      toast(error.message)
    }
  })
  $('permissionPicker').addEventListener('click', event => { event.preventDefault(); event.stopPropagation(); showPermissionMenu() })
  document.addEventListener('click', event => { if (!event.target.closest('#permissionMenu') && !event.target.closest('#permissionPicker')) $('permissionMenu').classList.add('hidden') })
  async function loadModelConfig() { try { state.modelConfig = await nativeInvoke('model_groups', {}) } catch (_) { state.modelConfig = null } }
  async function loadArchiveState() { try { const result = await nativeInvoke('session_archive', { action: 'list' }); state.archivedIds = new Set(result?.sessionIds || []) } catch (_) { state.archivedIds = new Set() } }
  (async function boot () { await loadModelConfig(); await loadArchiveState(); await refreshSessions(); consumeEvents() })()
})()
