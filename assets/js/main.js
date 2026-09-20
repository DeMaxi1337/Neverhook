/* Neverhook — header state, mobile nav, reveal, i18n.
   IMPORTANT: English is the source of truth and is read straight from index.html.
   To change any English text, just edit it in index.html — it will NOT be overwritten.
   Only Russian translations live in this file (RU / RU_HTML below). */
(function(){
  "use strict";
  var y=document.getElementById("year"); if(y) y.textContent=new Date().getFullYear();

  var hdr=document.getElementById("hdr");
  var onScroll=function(){hdr.classList.toggle("scrolled",window.scrollY>8);};
  onScroll(); window.addEventListener("scroll",onScroll,{passive:true});

  var burger=document.getElementById("burger"), nav=document.getElementById("nav");
  if(burger&&nav){
    burger.addEventListener("click",function(){nav.classList.toggle("open");});
    nav.querySelectorAll("a").forEach(function(a){a.addEventListener("click",function(){nav.classList.remove("open");});});
  }

  var reveals=document.querySelectorAll(".reveal");
  if("IntersectionObserver" in window){
    var io=new IntersectionObserver(function(es){es.forEach(function(e){if(e.isIntersecting){e.target.classList.add("in");io.unobserve(e.target);}});},{threshold:0.12,rootMargin:"0px 0px -6% 0px"});
    reveals.forEach(function(el){io.observe(el);});
  } else { reveals.forEach(function(el){el.classList.add("in");}); }

  // ---- i18n ----
  var textEls=document.querySelectorAll("[data-i18n]");
  var htmlEls=document.querySelectorAll("[data-i18n-html]");
  var EN_TEXT={}, EN_HTML={};
  textEls.forEach(function(el){var k=el.getAttribute("data-i18n"); if(!(k in EN_TEXT)) EN_TEXT[k]=el.textContent;});
  htmlEls.forEach(function(el){var k=el.getAttribute("data-i18n-html"); if(!(k in EN_HTML)) EN_HTML[k]=el.innerHTML;});

  // Russian translations only. Keys match data-i18n / data-i18n-html in index.html.
  var RU={
    "nav.features":"Модули",
    "nav.api":"Lua API",
    "nav.faq":"Вопросы",
    "faq.tag":"Вопросы",
    "faq.title":"Частые вопросы",
    "faq.q1":"Как установить мод через игру в Geode?",
    "faq.a1":"Откройте Geometry Dash, перейдите в меню Geode (логотип Geode на главном экране), нажмите на кнопку 'Поиск' (Download), введите название мода в строку поиска и нажмите 'Установить' (Install). После установки перезапустите игру, чтобы изменения вступили в силу.",
    "faq.q2":"Мод бесплатен?",
    "faq.a2":"Да, модификация абсолютно бесплатна для использования. Если она вам нравится, вы можете поддержать разработчика через ссылки в игре или на странице мода в Geode.",
    "faq.q3":"Игра вылетает после установки, что делать?",
    "faq.a3":"Убедитесь, что у вас установлена последняя версия Geometry Dash и самого Geode. Если проблема не исчезает, попробуйте отключить другие моды, чтобы выявить возможные конфликты, или обратитесь на сервер Discord разработчика за помощью.",
    "faq.q4":"Где найти сохраненные макросы или повторы?",
    "faq.a4":"Обычно файлы макросов и другие данные сохраняются в папке geode/mods/ или в специальной директории мода внутри папки с вашими сохранениями Geometry Dash (в %localappdata%\\GeometryDash на Windows).",
    "faq.q5":"Как добавить горячие клавиши (keybinds)?",
    "faq.a5":"В меню мода найдите нужную функцию и нажмите на нее правой кнопкой мыши или используйте кнопку с шестеренкой рядом с ней, чтобы назначить удобную клавишу для быстрого доступа.",
    "cta.get":"Скачать",
    "cta.explore":"Смотреть функции",
    "hero.eyebrow":"Geode · Geometry Dash 2.2081",
    "hero.meta":"Windows · Geometry Dash 2.2081 · Работает на Geode",
    "hero.sub":"Быстрое мод-меню для игры, практики и создания уровней — все основные функции всегда под рукой.",
    "features.tag":"Модули",
    "card.fx.t":"Полный арсенал",
    "card.fx.d":"40+ модулей в одном меню — байпасы, TAS-тулзы, косметика, хитбоксы и стартпоз-инструменты.",
    "card.bot.t":"Macrobot",
    "card.bot.d":"Встроенный TAS-бот. Запись и воспроизведение ввода с точностью до кадра — строй, тестируй и шлифуй любой прогон.",
    "card.lua.t":"Lua API",
    "card.lua.soon":"Скоро",
    "card.lua.d":"Открытый слой скриптинга на LuaJIT — пиши свои тулы или запускай скрипты сообщества. В разработке.",
    "footer.note":"Независимое мод-меню на Geode. Не связан с RobTop Games.",
    "footer.made":"Сделано DeMaxi"
  };
  var RU_HTML={
    "hero.title":"Всё-в-одном мод-меню<br/><span>для Geometry Dash.</span>",
    "features.title":"Каждый модуль <span class='mut'>ощущается частью</span> игры."
  };

  function applyLang(lang){
    var ru = lang==="ru";
    document.documentElement.setAttribute("lang",lang);
    textEls.forEach(function(el){var k=el.getAttribute("data-i18n"); el.textContent=(ru&&RU[k]!=null)?RU[k]:EN_TEXT[k];});
    htmlEls.forEach(function(el){var k=el.getAttribute("data-i18n-html"); el.innerHTML=(ru&&RU_HTML[k]!=null)?RU_HTML[k]:EN_HTML[k];});
    document.querySelectorAll(".lang-btn").forEach(function(b){b.classList.toggle("is-active",b.getAttribute("data-lang")===lang);});
    try{localStorage.setItem("nh-lang",lang);}catch(e){}
  }
  document.querySelectorAll(".lang-btn").forEach(function(b){b.addEventListener("click",function(){applyLang(b.getAttribute("data-lang"));});});
  var saved="en"; try{saved=localStorage.getItem("nh-lang")||"en";}catch(e){}
  applyLang(saved);
})();

window.toggleFaq = function(el) {
  var item = el.parentElement;
  var content = item.querySelector('.faq-content');
  if (item.classList.contains('active')) {
    item.classList.remove('active');
    content.style.maxHeight = null;
  } else {
    item.classList.add('active');
    content.style.maxHeight = content.scrollHeight + "px";
  }
};
