(function () {
  'use strict'
  const wallpapers = [
    { id: 'gothic-void', name: 'Gothic Void', file: 'gothic-void.jpg' },
    { id: 'red-white', name: 'Future City', file: 'red-white.png' },
    { id: 'alpine-night', name: 'Alpine Night', file: 'alpine-night.jpg' },
    { id: 'misty-peaks', name: 'Misty Peaks', file: 'misty-peaks.jpg' },
  ]
  const $ = id => document.getElementById(id)
  const settings = {
    theme: localStorage.getItem('dsh.theme') || 'dark',
    wallpaper: localStorage.getItem('dsh.wallpaper') === '1',
    wallpaperId: localStorage.getItem('dsh.wallpaperId') || 'gothic-void',
  }
  function apply() {
    document.body.dataset.theme = settings.theme
    document.body.classList.toggle('wallpaper-enabled', settings.wallpaper)
    const selected = wallpapers.find(item => item.id === settings.wallpaperId) || wallpapers[0]
    document.body.style.setProperty('--wallpaper-image', `url("wallpapers/${selected.file}")`)
    document.querySelectorAll('[data-theme-choice]').forEach(button => button.classList.toggle('active', button.dataset.themeChoice === settings.theme))
    const toggle = $('wallpaperEnabled'); if (toggle) toggle.checked = settings.wallpaper
    document.querySelectorAll('.wallpaper-choice').forEach(button => button.classList.toggle('active', button.dataset.wallpaper === selected.id))
  }
  function renderChoices() {
    const container = $('wallpaperChoices'); if (!container) return
    container.replaceChildren()
    wallpapers.forEach(item => { const button = document.createElement('button'); button.className = 'wallpaper-choice'; button.dataset.wallpaper = item.id; button.style.backgroundImage = `url("wallpapers/${item.file}")`; const label = document.createElement('span'); label.textContent = item.name; button.appendChild(label); button.addEventListener('click', () => { settings.wallpaperId = item.id; settings.wallpaper = true; localStorage.setItem('dsh.wallpaperId', item.id); localStorage.setItem('dsh.wallpaper', '1'); apply() }); container.appendChild(button) })
  }
  document.addEventListener('DOMContentLoaded', () => { renderChoices(); apply(); document.querySelectorAll('[data-theme-choice]').forEach(button => button.addEventListener('click', () => { settings.theme = button.dataset.themeChoice; localStorage.setItem('dsh.theme', settings.theme); apply() })); $('wallpaperEnabled')?.addEventListener('change', event => { settings.wallpaper = event.target.checked; localStorage.setItem('dsh.wallpaper', settings.wallpaper ? '1' : '0'); apply() }) })
})()
