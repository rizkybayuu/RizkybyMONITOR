        // Global JS Tooltips
        window.hardwareTooltips = {
            header: "<strong>🎮 RizkybyMONITOR - Window & Dashboard Controls:</strong><br>• <strong>Super + Left-Click Drag</strong> (or Header Drag): Move / drag frameless window<br>• <strong>Super + Right-Click Drag</strong> (or Alt + Right-Click Drag): Resize frameless window<br>• <strong>Left Click on Card:</strong> Toggle Fullscreen Zoom Mode<br>• <strong>Right Click on Card:</strong> Toggle Detailed CLI Diagnostic Text Mode<br>• <strong>Middle Click on Card:</strong> Copy visible card text to Clipboard<br>• <strong>Middle Click on Title:</strong> Copy full hover tooltip info to Clipboard<br>• <strong>Mouse Wheel on Color Selector:</strong> Quick cycle color palettes<br>• <strong>Left Click on Color Selector:</strong> Open color palette dropdown menu<br>• <strong>Mouse Wheel on Theme Toggle:</strong> Quick toggle Dark 🌙 / Light ☀️ mode<br>• <strong>Ctrl + Mouse Wheel:</strong> Adjust UI zoom & base font size<br>• <strong>Ctrl + N:</strong> Duplicate window into new instance<br>• <strong>Ctrl + Shift + A:</strong> Toggle Always-On-Top (Pin above windows)<br>• <strong>Ctrl + Q:</strong> Quit all windows and save layout<br>• <strong>Alt + Q:</strong> Exit current window and clear its local session memory<br>• <strong>Alt + S:</strong> Cycle autoscroll mode (▶ Always Active / ⏸ Smart / ■ Disabled)<br>• <strong>Alt + F:</strong> Open/close floating Telemetry Refresh Interval settings panel (min 500ms)",
            cpu: "Loading CPU Model...",
            gpu: "Loading GPU Model...",
            gpu_select: "<strong>🎮 GPU Selector:</strong><br>Select between active integrated graphics (iGPU) and discrete/external graphics cards (eGPU NVIDIA / AMD).",
            disk_select: "<strong>💽 Disk Selector:</strong><br>Choose a connected storage drive (NVMe SSD, SATA, USB External) for real-time I/O monitoring.",
            ram: "Loading RAM Details...",
            ram_cache: "<strong>🟦 CPU Smart Cache (~1.5 TB/s):</strong><br>Ultra-fast L1/L2/L3 hardware cache integrated directly on CPU die for zero latency.",
            ram_vram: "<strong>🟪 Dedicated VRAM (~300-800 GB/s):</strong><br>High-speed GDDR6/HBM video memory on discrete/external GPU.",
            ram_phys: "<strong>🟩 Physical System RAM (~40 GB/s):</strong><br>Main volatile system memory pool.",
            ram_apps: "<strong>🟩 Used by Apps:</strong><br>Memory consumed by active user applications.",
            ram_cached: "<strong>🟨 Buffers / File Cache:</strong><br>Page cache automatically managed by kernel.",
            ram_shared: "<strong>🟪 Shared VRAM:</strong><br>System RAM shared for integrated graphics.",
            zram: "Loading ZRAM Details...",
            swap: "Loading Swap Details...",
            ssd: "Loading Storage Details...",
            battery: "Loading Battery Status...",
            temperature: "Loading Thermal Sensors...",
            mode_toggle: "<strong>🌙 / ☀️ Theme Mode Toggle:</strong><br>Switch between Dark Mode and Light Mode with independent color palettes per window.",
            color_select: "<strong>🎨 Color Palette Selector:</strong><br>Choose a custom vibrant color palette for the current window.",
            network: "Loading Network Details...",
            autoscroll_ctrl: "<strong>📜 Autoscroll & Telemetry Refresh Controls:</strong><br>• <strong>Left Click / Alt+S:</strong> Cycle autoscroll mode (▶ Always Active / ⏸ Smart / ■ Disabled)<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<strong>▶ Play:</strong> Autoscroll is always active<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<strong>⏸ Pause:</strong> Smart autoscroll (active window or hover interaction)<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<strong>■ Stop:</strong> Autoscroll completely disabled (GPU saving)<br>• <strong>Right Click / Alt+F:</strong> Open/close floating Telemetry Refresh Interval settings panel (min 500ms)",
            duplicate: "<strong>🗗 Duplicate Controls:</strong><br>• <strong>Left Click / Ctrl+N:</strong> Duplicate Window<br>• <strong>Right Click / Ctrl+Shift+A:</strong> Toggle Keep Above Other (Pin Window)",
            pinned: "<strong>🖈 Pin Controls (Always On Top: ENABLED):</strong><br>• <strong>Left Click / Ctrl+N:</strong> Duplicate Window<br>• <strong>Right Click / Ctrl+Shift+A:</strong> Unpin / Disable Always-On-Top",
            quit: "<strong>⏻ Quit Controls:</strong><br>• <strong>Left Click / Ctrl+Q:</strong> Quit All Windows & Save Layout<br>• <strong>Right Click / Alt+Q:</strong> Exit Current Window & Remove Memory"
        };

        let globalTooltip = document.createElement('div');
        globalTooltip.id = 'globalTooltip'; // Pastikan ID terpasang agar selector CSS bekerja
        globalTooltip.style.position = 'absolute';
        globalTooltip.style.zIndex = '150000';
        globalTooltip.style.padding = '1.5vh 1.5vw';
        globalTooltip.style.borderRadius = '8px';
        globalTooltip.style.fontSize = 'clamp(0.75rem, 0.9vw, 0.9rem)';
        globalTooltip.style.opacity = '0';
        globalTooltip.style.pointerEvents = 'none';
        globalTooltip.style.whiteSpace = 'normal';
        globalTooltip.style.lineHeight = '1.5';
        document.getElementById('app-window').appendChild(globalTooltip);

        let currentTooltipKey = null;

        document.addEventListener('mouseover', (e) => {
            let target = e.target.closest('[data-tooltip]');
            if (target) {
                // Cegah tooltip muncul jika elemen judul sedang diedit
                if (target.id === 'header-app-title' && target.contentEditable === "true") return;

                window._lastHoveredTooltipEl = target; // REKAM ELEMEN ASLI (Baterai, Quit, Mode, dsb.)

                let key = target.getAttribute('data-tooltip');
                currentTooltipKey = key;
                globalTooltip.innerHTML = window.hardwareTooltips[key] || key;

                // Samaratakan border untuk SEMUA tooltip
                globalTooltip.style.borderColor = 'var(--glass-border)';

                globalTooltip.style.opacity = '1';
            }
        });

        document.addEventListener('mousemove', (e) => {
            let target = e.target.closest('[data-tooltip]');
            if (target && globalTooltip.style.opacity === '1') {
                let key = target.getAttribute('data-tooltip');
                if (window.hardwareTooltips[key]) {
                    globalTooltip.innerHTML = window.hardwareTooltips[key];
                }

                let x = e.pageX - globalTooltip.offsetWidth - 15;
                let y = e.pageY - globalTooltip.offsetHeight - 15;

                if (x < 10) x = e.pageX + 15;
                if (y < 10) y = e.pageY + 15;

                globalTooltip.style.left = x + 'px';
                globalTooltip.style.top = y + 'px';

                // Prioritaskan Tooltip Header: Z-Index di atas tombol GitHub (999999) dan bebas masking
                // Prioritaskan Tooltip Header: Z-Index di atas tombol GitHub (999999) dan bebas masking
                if (target.closest('header')) {
                    globalTooltip.style.zIndex = '1000001';
                    globalTooltip.style.webkitMaskImage = '';
                    globalTooltip.style.maskImage = '';
                } else {
                    globalTooltip.style.zIndex = '150000';
                    if (isEasterEggActive) {
                        applyPopupMask(globalTooltip);
                    } else {
                        globalTooltip.style.webkitMaskImage = '';
                        globalTooltip.style.maskImage = '';
                    }
                }
            }
        });

        document.addEventListener('mouseout', (e) => {
            let target = e.target.closest('[data-tooltip]');
            if (target) {
                globalTooltip.style.opacity = '0';
                window._lastHoveredTooltipEl = null;
            }
        });

        const urlParams = new URLSearchParams(window.location.search);
        const windowId = urlParams.get('win') || '1';

        const safeStorage = {
            _memory: {},
            setItem: function(key, value) {
                try {
                    localStorage.setItem(key, value);
                } catch (e) {
                    this._memory[key] = String(value);
                }
            },
            getItem: function(key) {
                try {
                    return localStorage.getItem(key);
                } catch (e) {
                    return this._memory[key] || null;
                }
            },
            removeItem: function(key) {
                try {
                    localStorage.removeItem(key);
                } catch (e) {
                    delete this._memory[key];
                }
            }
        };

        let _renderedCredits = false;

        let isEasterEggActive = false;
        let isEasterTransitioning = false;
        let easterEggClone = null;
        let easterMaskBanner = null;
        let easterEffectLayer = null;
        let originalGithubBtn = null;
        let originalGithubRect = null;

        // Fungsi pengunci scroll credit-frozen SELALU MENTOK DI PALING BAWAH (Rasio 1)
        function scrollCreditFrozenToBottom() {
            const frozenEl = document.getElementById('credit-frozen');
            if (frozenEl) {
                const maxScroll = Math.max(0, frozenEl.scrollHeight - frozenEl.clientHeight);
                frozenEl.scrollTop = maxScroll;
            }
        }

        window.addEventListener('resize', scrollCreditFrozenToBottom);
        document.addEventListener('DOMContentLoaded', scrollCreditFrozenToBottom);

        // 1. Fungsi pengukur area utama yang memperhitungkan padding bottom body (12px)
        function getMainContentAreaRect() {
            // Ambil koordinat dan dimensi langsung dari .dashboard asli
            const dashboardEl = document.querySelector('.dashboard');
            if (dashboardEl) {
                const dRect = dashboardEl.getBoundingClientRect();
                return {
                    top: dRect.top,
                    left: dRect.left,
                    width: dRect.width,
                    height: dRect.height
                };
            }

            // Fallback jika .dashboard sedang di-hide/display:none
            const headerEl = document.querySelector('header');
            const headerRect = headerEl ? headerEl.getBoundingClientRect() : { bottom: 0 };
            const bodyPadding = 12;
            const bodyGap = 8;

            return {
                top: headerRect.bottom + bodyGap,
                left: bodyPadding,
                width: window.innerWidth - (bodyPadding * 2),
                height: window.innerHeight - (headerRect.bottom + bodyGap) - bodyPadding
            };
        }

        // Variabel & Mesin Kontrol Scroll Credit Kedua
        let easterCreditContainer = null;
        let easterScrollAnimId = null;
        let easterScrollTimer = null;
        let isEasterManualScrolling = false;
        let easterManualResumeTimer = null;

        function stopEasterCreditsScroll() {
            if (easterScrollTimer) { clearTimeout(easterScrollTimer); easterScrollTimer = null; }
            if (easterScrollAnimId) { cancelAnimationFrame(easterScrollAnimId); easterScrollAnimId = null; }
        }

        function startEasterCreditsScroll() {
            stopEasterCreditsScroll();
            stopChangelogHScroll(true);
            if (!easterCreditContainer || !isEasterEggActive || !isAutoscrollAllowed()) return;

            isEasterManualScrolling = false;

            if (!easterCreditContainer._hasManualScrollListener) {
                easterCreditContainer._hasManualScrollListener = true;

                const handleManualScroll = (e) => {
                    isEasterManualScrolling = true;
                    stopEasterCreditsScroll();

                    if (e && e.deltaY) {
                        easterCreditContainer.scrollTop += e.deltaY * 0.5;
                    }

                    if (easterManualResumeTimer) clearTimeout(easterManualResumeTimer);
                    easterManualResumeTimer = setTimeout(() => {
                        isEasterManualScrolling = false;
                        resumeEasterCreditsScroll();
                    }, 4000);
                };

                easterCreditContainer.addEventListener('wheel', handleManualScroll, { passive: false });
                easterCreditContainer.addEventListener('touchmove', handleManualScroll, { passive: true });
            }

            resumeEasterCreditsScroll();
        }

        function resumeEasterCreditsScroll() {
            if (easterScrollAnimId) cancelAnimationFrame(easterScrollAnimId);
            const panel = easterCreditContainer;
            if (!panel || isEasterManualScrolling || !isEasterEggActive || !isAutoscrollAllowed()) return;

            const maxScroll = panel.scrollHeight - panel.clientHeight;
            if (maxScroll <= 0) return;

            const startPos = panel.scrollTop;
            if (startPos >= maxScroll - 2) {
                fastScrollEasterToTop(panel, () => {
                    startEasterCreditsScroll();
                });
                return;
            }

            const targetLinearSpeed = 65;
            const easeDuration = 1000;
            const initialDelay = 2000;

            const easeDistance = 0.5 * targetLinearSpeed * (easeDuration / 1000);
            const totalRemaining = maxScroll - startPos;
            const linearDistance = Math.max(0, totalRemaining - (2 * easeDistance));
            const linearDuration = (linearDistance / targetLinearSpeed) * 1000;
            const totalTripDuration = initialDelay + (2 * easeDuration) + linearDuration;

            let startTime = null;

            function scrollDownStep(timestamp) {
                if (isEasterManualScrolling || !isEasterEggActive || !isAutoscrollAllowed()) return;
                if (!startTime) startTime = timestamp;
                const elapsed = timestamp - startTime;

                let currentScroll = startPos;

                if (elapsed < initialDelay) {
                    currentScroll = startPos;
                } else if (elapsed < initialDelay + easeDuration) {
                    const t = (elapsed - initialDelay) / easeDuration;
                    currentScroll = startPos + (easeDistance * t * t);
                } else if (elapsed < initialDelay + easeDuration + linearDuration) {
                    const linearElapsed = (elapsed - initialDelay - easeDuration) / 1000;
                    currentScroll = startPos + easeDistance + (linearElapsed * targetLinearSpeed);
                } else if (elapsed < totalTripDuration) {
                    const t = (elapsed - initialDelay - easeDuration - linearDuration) / easeDuration;
                    const decelFactor = t * (2 - t);
                    currentScroll = startPos + easeDistance + linearDistance + (easeDistance * decelFactor);
                } else {
                    currentScroll = maxScroll;
                }

                if (currentScroll < maxScroll && elapsed < totalTripDuration) {
                    panel.scrollTop = currentScroll;
                    easterScrollAnimId = requestAnimationFrame(scrollDownStep);
                } else {
                    panel.scrollTop = maxScroll;
                    easterScrollTimer = setTimeout(() => {
                        fastScrollEasterToTop(panel, () => {
                            startEasterCreditsScroll();
                        });
                    }, 2500);
                }
            }

            easterScrollAnimId = requestAnimationFrame(scrollDownStep);
        }

        function fastScrollEasterToTop(panel, onComplete) {
            const startPos = panel.scrollTop;
            const duration = 700;
            let startTime = null;
            const easeInOutQuad = t => t < 0.5 ? 2 * t * t : -1 + (4 - 2 * t) * t;

            function scrollUpStep(timestamp) {
                if (!startTime) startTime = timestamp;
                const elapsed = timestamp - startTime;
                const t = Math.min(1, elapsed / duration);
                panel.scrollTop = startPos * (1 - easeInOutQuad(t));

                if (t < 1) {
                    easterScrollAnimId = requestAnimationFrame(scrollUpStep);
                } else {
                    panel.scrollTop = 0;
                    if (onComplete) onComplete();
                }
            }
            easterScrollAnimId = requestAnimationFrame(scrollUpStep);
        }

        // Helper: Terapkan batas gradien blur & interpolasi fade in/out secara kontinu
        function setEasterEffectMask(btnY, btnHeight, fadeFactor = 1) {
            if (!easterEffectLayer) return;
            const headerEl = document.querySelector('header');
            const headerRect = headerEl ? headerEl.getBoundingClientRect() : { top: 0, height: 50, bottom: 50 };
            const headerCenter = headerRect.top + (headerRect.height / 2);
            const githubCenter = btnY + (btnHeight / 2);

            easterEffectLayer.style.top = '0px';
            easterEffectLayer.style.height = '100vh';

            const maskGradient = `linear-gradient(to bottom, transparent 0px, transparent ${headerCenter}px, black ${githubCenter}px, black 100%)`;
            easterEffectLayer.style.webkitMaskImage = maskGradient;
            easterEffectLayer.style.maskImage = maskGradient;

            const clampedFactor = Math.max(0, Math.min(1, fadeFactor));
            const blurRadius = (8 * clampedFactor).toFixed(2);
            easterEffectLayer.style.backdropFilter = `blur(${blurRadius}px)`;
            easterEffectLayer.style.webkitBackdropFilter = `blur(${blurRadius}px)`;
            easterEffectLayer.style.opacity = clampedFactor.toFixed(3);

            // BACA KONDISI MODE SECARA REALTIME
            const isLight = document.documentElement.getAttribute('data-mode') === 'light' ||
                            document.body.getAttribute('data-mode') === 'light' ||
                            currentThemeMode === 'light';

            if (isLight) {
                // LIGHT MODE = POSITIF BRIGHTNESS (Putih Transparan)
                easterEffectLayer.style.backgroundColor = `rgba(255, 255, 255, ${(0.40 * clampedFactor).toFixed(3)})`;
            } else {
                // DARK MODE = MINUS BRIGHTNESS (Hitam Transparan)
                easterEffectLayer.style.backgroundColor = `rgba(0, 0, 0, ${(0.50 * clampedFactor).toFixed(3)})`;
            }
        }

        function updateEasterEggPositions() {
            if (!isEasterEggActive || !easterEggClone) {
                if (easterEffectLayer) {
                    easterEffectLayer.style.webkitMaskImage = '';
                    easterEffectLayer.style.maskImage = '';
                }
                return;
            }

            if (isEasterTransitioning) return;

            const headerEl = document.querySelector('header');
            const fontBase = parseFloat(getComputedStyle(document.documentElement).fontSize) || 16;
            const headerRect = headerEl ? headerEl.getBoundingClientRect() : { top: 0, height: 50, bottom: 50 };
            const githubTop = headerRect.bottom + (0.5 * fontBase);

            easterEggClone.style.transition = 'none';
            easterEggClone.style.width = 'max-content';
            easterEggClone.style.height = 'auto';
            easterEggClone.style.top = githubTop + 'px';
            easterEggClone.style.left = '50vw';
            easterEggClone.style.transform = 'translateX(-50%)';

            const githubHeight = easterEggClone.offsetHeight || (2.5 * fontBase);
            setEasterEffectMask(githubTop, githubHeight);

            if (easterCreditContainer) {
                easterCreditContainer.style.top = (githubTop + githubHeight) + 'px';
            }
        }

        window.addEventListener('resize', updateEasterEggPositions);

        // Hapus listener wheel ganda di baris 810, sinkronkan langsung di dalam handler Ctrl+Wheel utama

        let easterOpenedFromAbout = false;

                document.addEventListener('contextmenu', function(e) {
            // Tangkap elemen GitHub meskipun klik kanan mengenai anak elemen (SVG/Span)
            const githubBtn = e.target.closest('.github-link') || e.target.closest('.easter-github-clone') || (e.target.tagName === 'A' && e.target.classList.contains('github-link') ? e.target : null);
            if (!githubBtn) return;

            e.preventDefault();
            e.stopPropagation();

            // KUNCI: Blokir klik kanan jika sedang dalam transisi animasi (faktor 0-1 atau 1-0)
            if (isEasterTransitioning) return;

            if (!isEasterEggActive) {
                // ========================================================
                // === TOGGLE ON (FALSE -> TRUE): LUNCURKAN ANIMASI ===
                // ========================================================
                isEasterEggActive = true;
                easterOpenedFromAbout = !easterOpenedFromAbout;
                originalGithubBtn = githubBtn;

                scrollCreditFrozenToBottom();

                if (easterOpenedFromAbout && !aboutScrollAnimId) {
                    startAboutCreditsScroll();
                }

                // 2. BARULAH ANIMASI TOMBOL GITHUB DIJALANKAN
                const r = originalGithubBtn.getBoundingClientRect();

                // Layer Blur
                easterEffectLayer = document.createElement('div');
                easterEffectLayer.className = 'easter-effect-layer';
                document.getElementById('app-window').appendChild(easterEffectLayer);

                // Container Konten
                easterCreditContainer = document.createElement('div');
                easterCreditContainer.className = 'easter-credit-wrapper';
                easterCreditContainer.id = 'easter-credit-container';
                easterCreditContainer.style.transition = 'none';
                easterCreditContainer.style.top = (r.top + r.height) + 'px';
                easterCreditContainer.style.opacity = '0';
                document.getElementById('app-window').appendChild(easterCreditContainer);

                updateEasterEggContent(true);

                const innerDiv = easterCreditContainer.firstElementChild;
                if (innerDiv) {
                    innerDiv.style.animation = 'none';
                }

                // Clone Tombol Melayang
                easterEggClone = document.createElement('a');
                easterEggClone.className = 'github-link easter-github-clone';
                easterEggClone.href = "https://github.com/rizkybayuu";
                easterEggClone.setAttribute('target', '_blank');
                easterEggClone.setAttribute('rel', 'noopener noreferrer');

                // KLIK KIRI SAJA YANG BUKA LINK WEBSITE
                easterEggClone.onclick = function(ev) {
                    ev.preventDefault();
                    ev.stopPropagation();
                    // Blokir klik jika sedang dalam transisi meluncur
                    if (isEasterTransitioning) return;
                    openExternalLink(this.href);
                };

                // KLIK KANAN MENIRU SIFAT ASLI: HANYA TOGGLE, JANGAN BUKA WEBSITE
                easterEggClone.oncontextmenu = function(ev) {
                    ev.preventDefault();
                    ev.stopPropagation();
                    // Tidak panggil openExternalLink sama sekali!
                };
                easterEggClone.innerHTML = `
                    <svg class="github-icon" viewBox="0 0 24 24" aria-hidden="true">
                        <path d="M12 0C5.37 0 0 5.37 0 12c0 5.31 3.435 9.795 8.205 11.385.6.105.825-.255.825-.57 0-.285-.015-1.23-.015-2.235-3.015.555-3.795-.735-4.035-1.41-.135-.345-.72-1.41-1.23-1.695-.42-.225-1.02-.78-.015-.795.945-.015 1.62.87 1.845 1.23 1.08 1.815 2.805 1.305 3.495.99.105-.78.42-1.305.765-1.605-2.67-.3-5.46-1.335-5.46-5.925 0-1.305.465-2.385 1.23-3.225-.12-.3-.54-1.53.12-3.18 0 0 1.005-.315 3.3 1.23.96-.27 1.98-.405 3-.405s2.04.135 3 .405c2.295-1.56 3.3-1.23 3.3-1.23.66 1.65.24 2.88.12 3.18.765.84 1.23 1.905 1.23 3.225 0 4.605-2.805 5.625-5.475 5.925.435.375.81 1.095.81 2.22 0 1.605-.015 2.895-.015 3.3 0 .315.225.69.825.57A12.02 12.02 0 0024 12c0-6.63-5.37-12-12-12z"/>
                    </svg>
                    <span>github.com/rizkybayuu</span>
                `;

                easterEggClone.style.position = 'fixed';
				easterEggClone.style.left = '50vw';
				easterEggClone.style.transform = 'translateX(-50%)';
				easterEggClone.style.top = r.top + 'px';
				easterEggClone.style.width = 'max-content';
				easterEggClone.style.height = 'auto';
				easterEggClone.style.margin = '0';
				easterEggClone.style.zIndex = '500000';
				easterEggClone.style.pointerEvents = 'auto';
                easterEggClone.style.cursor = 'pointer';
                easterEggClone.style.transition = 'none';

                originalGithubBtn.style.opacity = '0';
                document.getElementById('app-window').appendChild(easterEggClone);

                const headerEl = document.querySelector('header');
                const fontBase = parseFloat(getComputedStyle(document.documentElement).fontSize) || 16;
                const headerRect = headerEl ? headerEl.getBoundingClientRect() : { top: 0, height: 50, bottom: 50 };
                const targetTop = headerRect.bottom + (0.5 * fontBase);

                isEasterTransitioning = true;
                const duration = 550;
                const startY = r.top;
                const deltaY = targetTop - startY;
                const easeInOutCubic = t => t < 0.5 ? 4 * t * t * t : 1 - Math.pow(-2 * t + 2, 3) / 2;
                let launchStartTime = null;

                setEasterEffectMask(startY, r.height, 0);

                function launchStep(timestamp) {
                    if (!launchStartTime) launchStartTime = timestamp;
                    const elapsed = timestamp - launchStartTime;
                    const progress = Math.min(1, elapsed / duration);
                    const factor = easeInOutCubic(progress);

                    const curY = startY + (deltaY * factor);
                    easterEggClone.style.top = curY + 'px';

                    const curHeight = easterEggClone.offsetHeight || r.height;

                    setEasterEffectMask(curY, curHeight, factor);

                    if (easterCreditContainer) {
                        easterCreditContainer.style.top = (curY + curHeight) + 'px';
                        easterCreditContainer.style.opacity = factor.toFixed(3);
                    }

                    if (progress < 1) {
                        requestAnimationFrame(launchStep);
                    } else {
                        isEasterTransitioning = false;
                        document.body.classList.add('easter-egg-active');
                        updateEasterEggPositions();
                        startEasterCreditsScroll();
                    }
                }

                requestAnimationFrame(launchStep);

                        } else {
                // ========================================================
                // === TOGGLE OFF (TRUE -> FALSE): ANIMASI LUNCUR RETUR ===
                // ========================================================
                isEasterEggActive = false;

                // 1. Matikan mode fullscreen jika sedang aktif
                document.querySelectorAll('.card.fullscreen').forEach(c => c.classList.remove('fullscreen'));
                const fsOverlayEl = document.getElementById('fs-overlay');
                if (fsOverlayEl) fsOverlayEl.classList.remove('active');
                if (typeof resyncChartFonts === 'function') resyncChartFonts();
                safeStorage.setItem(`rizkyby_${windowId}_fullscreen`, '');
                saveConfig({ fullscreen: '' });

                // 2. Hentikan autoscroll
                stopEasterCreditsScroll();
                stopAboutAutoScroll();
                if (aboutScrollTimer) { clearTimeout(aboutScrollTimer); aboutScrollTimer = null; }
                if (aboutScrollAnimId) { cancelAnimationFrame(aboutScrollAnimId); aboutScrollAnimId = null; }
                if (aboutManualResumeTimer) { clearTimeout(aboutManualResumeTimer); aboutManualResumeTimer = null; }

                const isCurrentlyAbout = document.body.classList.contains('show-about-panel');
                const aboutPanel = document.getElementById('about-panel');
                const frozenGithubBtn = document.querySelector('#credit-frozen .github-link');

                // Pastikan posisi credit-frozen selalu mentok di maxScroll (1)
                scrollCreditFrozenToBottom();

                // Ambil koordinat Y landing dari tombol GitHub di credit-frozen
                let targetY = window.innerHeight - 80;
                if (frozenGithubBtn) {
                    const fRect = frozenGithubBtn.getBoundingClientRect();
                    if (fRect.top > 0) targetY = fRect.top;
                }

                if (isCurrentlyAbout) {
                    // =========================================================================
                    // CONDITIONAL 1: RIZKYBYMONITOR TOGGLE KLIK KANAN = TRUE
                    // Meluncur dari TOP ke lokasi tombol GitHub credit-frozen (500ms)
                    // =========================================================================
                    isEasterTransitioning = true;
                    const duration = 500;
                    const startY = easterEggClone ? easterEggClone.getBoundingClientRect().top : 50;
                    const deltaY = targetY - startY;
                    const startScrollTop = aboutPanel ? aboutPanel.scrollTop : 0;
                    const maxScroll = aboutPanel ? Math.max(0, aboutPanel.scrollHeight - aboutPanel.clientHeight) : 0;
                    const deltaScroll = maxScroll - startScrollTop;
                    const easeInOutCubic = t => t < 0.5 ? 4 * t * t * t : 1 - Math.pow(-2 * t + 2, 3) / 2;
                    let animStartTime = null;

                    function returnStepCond1(timestamp) {
                        if (!animStartTime) animStartTime = timestamp;
                        const elapsed = timestamp - animStartTime;
                        const progress = Math.min(1, elapsed / duration);
                        const factor = easeInOutCubic(progress);

                        // A. Scroll credit utama menuju maxScroll (1)
                        if (aboutPanel && deltaScroll !== 0) {
                            aboutPanel.scrollTop = startScrollTop + (deltaScroll * factor);
                        }

                        // B. Meluncurkan tombol GitHub turun ke lokasi credit-frozen
                        const curY = startY + (deltaY * factor);
                        if (easterEggClone) {
                            easterEggClone.style.top = curY + 'px';
                        }

                        // C. Efek Fade Out Opacity & Blur
                        const outFactor = Math.max(0, 1 - factor);
                        const curHeight = easterEggClone ? (easterEggClone.offsetHeight || 40) : 40;
                        setEasterEffectMask(curY, curHeight, outFactor);

                        if (easterCreditContainer) {
                            easterCreditContainer.style.top = (curY + curHeight) + 'px';
                            easterCreditContainer.style.opacity = outFactor.toFixed(3);
                        }

                        if (progress < 1) {
                            requestAnimationFrame(returnStepCond1);
						} else {
							// SELESAI
							isEasterTransitioning = false;
							document.body.classList.remove('easter-egg-active');
							if (aboutPanel) aboutPanel.scrollTop = maxScroll;

							if (easterEggClone) { easterEggClone.remove(); easterEggClone = null; }
							if (easterEffectLayer) { easterEffectLayer.remove(); easterEffectLayer = null; }
							if (easterCreditContainer) { easterCreditContainer.remove(); easterCreditContainer = null; }

							if (originalGithubBtn) {
								originalGithubBtn.style.opacity = '1';
							}

							// Tahan selama 2 detik di titik bawah, lalu lanjut autoscroll / scroll to top
							if (aboutScrollTimer) clearTimeout(aboutScrollTimer);
							aboutScrollTimer = setTimeout(() => {
								startAboutCreditsScroll();
							}, 2000);
						}
                    }

                    requestAnimationFrame(returnStepCond1);

                } else {
                    // =========================================================================
                    // CONDITIONAL 2: RIZKYBYMONITOR TOGGLE KLIK KANAN = FALSE (DARI DASHBOARD)
                    // Durasi Total: 800ms (300ms transisi dashboard -> about + 500ms scroll max 1)
                    // =========================================================================
                    const dashboardEl = document.querySelector('.dashboard');
                    isEasterTransitioning = true;

                    // a. Animasi transisi RizkybyMONITOR false -> true (300ms)
                    // a. Animasi transisi RizkybyMONITOR false -> true (300ms)
                    if (dashboardEl) {
                        dashboardEl.classList.remove('cards-animate-in');
                        dashboardEl.classList.add('cards-hidden');
                    }

                    setTimeout(() => {
                        if (dashboardEl) dashboardEl.style.display = 'none';
                        document.body.classList.add('show-about-panel');
                        if (aboutPanel) {
                            aboutPanel.style.display = 'flex';
                            aboutPanel.classList.remove('about-hidden');
                            aboutPanel.scrollTop = 0;

                            // Animasi scroll credit utama 0 -> maxScroll 1 (500ms)
                            const maxScroll = Math.max(0, aboutPanel.scrollHeight - aboutPanel.clientHeight);
                            let sStart = null;
                            function animScroll(ts) {
                                if (!sStart) sStart = ts;
                                const prog = Math.min(1, (ts - sStart) / 500);
                                const ease = t => t < 0.5 ? 4 * t * t * t : 1 - Math.pow(-2 * t + 2, 3) / 2;
                                aboutPanel.scrollTop = maxScroll * ease(prog);
                                if (prog < 1) requestAnimationFrame(animScroll);
                            }
                            requestAnimationFrame(animScroll);
                        }
                    }, 300);

                    // b. Animasi tombol GitHub meluncur dari TOP ke lokasi credit-frozen (800ms)
                    const duration = 800;
                    const startY = easterEggClone ? easterEggClone.getBoundingClientRect().top : 50;
                    const deltaY = targetY - startY;
                    const easeInOutCubic = t => t < 0.5 ? 4 * t * t * t : 1 - Math.pow(-2 * t + 2, 3) / 2;
                    let animStartTime = null;

                    function returnStepCond2(timestamp) {
                        if (!animStartTime) animStartTime = timestamp;
                        const elapsed = timestamp - animStartTime;
                        const progress = Math.min(1, elapsed / duration);
                        const factor = easeInOutCubic(progress);

                        const curY = startY + (deltaY * factor);
                        if (easterEggClone) {
                            easterEggClone.style.top = curY + 'px';
                        }

                        const outFactor = Math.max(0, 1 - factor);
                        const curHeight = easterEggClone ? (easterEggClone.offsetHeight || 40) : 40;
                        setEasterEffectMask(curY, curHeight, outFactor);

                        if (easterCreditContainer) {
                            easterCreditContainer.style.top = (curY + curHeight) + 'px';
                            easterCreditContainer.style.opacity = outFactor.toFixed(3);
                        }

                        if (progress < 1) {
                            requestAnimationFrame(returnStepCond2);
                        } else {
                            // SELESAI
                            isEasterTransitioning = false;
                            document.body.classList.remove('easter-egg-active');

                            if (aboutPanel) {
                                const maxScroll = Math.max(0, aboutPanel.scrollHeight - aboutPanel.clientHeight);
                                aboutPanel.scrollTop = maxScroll;
                            }

                            if (easterEggClone) { easterEggClone.remove(); easterEggClone = null; }
                            if (easterEffectLayer) { easterEffectLayer.remove(); easterEffectLayer = null; }
                            if (easterCreditContainer) { easterCreditContainer.remove(); easterCreditContainer = null; }

                            if (originalGithubBtn) {
                                originalGithubBtn.style.opacity = '1';
                            }

                            // BERSIHKAN SEMUA TIMER/FLAG AGAR TIDAK TERKUNCI
                            isAboutManualScrolling = false;
                            if (aboutScrollTimer) clearTimeout(aboutScrollTimer);
                            if (aboutScrollAnimId) {
                                cancelAnimationFrame(aboutScrollAnimId);
                                aboutScrollAnimId = null;
                            }

                            // JEDA 2 DETIK DI TITIK BAWAH LALU NYALAKAN AUTOSCROLL KEMBALI
                            aboutScrollTimer = setTimeout(() => {
                                startAboutCreditsScroll();
                            }, 2000);
                        }
                    }

                    requestAnimationFrame(returnStepCond2);
                }
            }
        }, { capture: true });

        function updateAboutPanel(credits) {
            // Jika data tidak valid atau bukan objek dari backend, kosongkan total
            const pillsCont = document.getElementById('about-tech-pills');
            const contentCont = document.getElementById('about-dynamic-content');
            const frozenPillsCont = document.getElementById('frozen-tech-pills');
            const frozenContentCont = document.getElementById('frozen-dynamic-content');
            if (!pillsCont || !contentCont) return;

            if (!credits || !credits.valid || !Array.isArray(credits.sections)) {
                pillsCont.innerHTML = '';
                contentCont.innerHTML = '';
                _renderedCredits = false;
                return;
            }

            if (!credits || !credits.valid || !Array.isArray(credits.sections)) {
                pillsCont.innerHTML = '';
                contentCont.innerHTML = '';
                _renderedCredits = false;
                return;
            }
            if (_renderedCredits) {
                return;
            }
            _renderedCredits = true;

            // Render Tech Pills (Sinkron ke frozen header)
            const pillsHtml = (credits.pills || []).map(p =>
                `<span class="tech-pill">${p}</span>`
            ).join('');
            pillsCont.innerHTML = pillsHtml;
            if (frozenPillsCont) frozenPillsCont.innerHTML = pillsHtml;

            // Render Sections & Specs (Dengan aksen warna identik)
            let sectionsHtml = (credits.sections || []).map(sec => `
                <div class="about-section">
                    <div class="about-section-heading">${sec.heading}</div>
                    <div class="about-spec-list">
                        ${(sec.specs || []).map(item => {
                            let colorStyle = '';
                            if (item.label === 'Core Runtime') colorStyle = ' style="color:var(--accent-blue);"';
                            else if (item.label === 'Telemetry Pipeline') colorStyle = ' style="color:var(--accent-green);"';
                            else if (item.label === 'Storage Diagnostics') colorStyle = ' style="color:var(--accent-orange);"';
                            return `
                            <div class="about-spec-row">
                                <span class="about-spec-label">${item.label}</span>
                                <span class="about-spec-value"${colorStyle}>${item.val}</span>
                            </div>`;
                        }).join('')}
                    </div>
                </div>
            `).join('');

            // Bagian GitHub & Penutup
            sectionsHtml += `
                <div class="about-section">
                    <div class="about-section-heading">Source Repository</div>
                    <a class="github-link" href="https://github.com/rizkybayuu" target="_blank" rel="noopener noreferrer" onclick="event.preventDefault(); openExternalLink(this.href);">
                        <svg class="github-icon" viewBox="0 0 24 24" aria-hidden="true">
                            <path d="M12 0C5.37 0 0 5.37 0 12c0 5.31 3.435 9.795 8.205 11.385.6.105.825-.255.825-.57 0-.285-.015-1.23-.015-2.235-3.015.555-3.795-.735-4.035-1.41-.135-.345-.72-1.41-1.23-1.695-.42-.225-1.02-.78-.015-.795.945-.015 1.62.87 1.845 1.23 1.08 1.815 2.805 1.305 3.495.99.105-.78.42-1.305.765-1.605-2.67-.3-5.46-1.335-5.46-5.925 0-1.305.465-2.385 1.23-3.225-.12-.3-.54-1.53.12-3.18 0 0 1.005-.315 3.3 1.23.96-.27 1.98-.405 3-.405s2.04.135 3 .405c2.295-1.56 3.3-1.23 3.3-1.23.66 1.65.24 2.88.12 3.18.765.84 1.23 1.905 1.23 3.225 0 4.605-2.805 5.625-5.475 5.925.435.375.81 1.095.81 2.22 0 1.605-.015 2.895-.015 3.3 0 .315.225.69.825.57A12.02 12.02 0 0024 12c0-6.63-5.37-12-12-12z"/>
                        </svg>
                        github.com/rizkybayuu
                    </a>
                    <p class="closing-note">
                        Thank you for using this monitor application.<br>
                        Wish you all the best!
                    </p>
                </div>
            `;

            contentCont.innerHTML = sectionsHtml;
            if (frozenContentCont) frozenContentCont.innerHTML = sectionsHtml;
        }

        async function saveConfig(obj) {
            try {
                await fetch('/api/config', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ window_id: windowId, settings: obj })
                });
            } catch(e) {}
        }

        // Set chart default styling
        Chart.defaults.color = '#94a3b8';
        Chart.defaults.font.family = "'Outfit', sans-serif";
        
        function formatBytes(bytes) {
            if (!bytes || bytes <= 0) return '0';
            if (bytes < 1024 ** 2) return (bytes / 1024).toFixed(1) + ' KB';
            if (bytes < 1024 ** 3) return (bytes / (1024 ** 2)).toFixed(1) + ' MB';
            return (bytes / (1024 ** 3)).toFixed(2) + ' GB';
        }

        function formatSpeed(bytesPerSec) {
            const b = Number(bytesPerSec) || 0;
            if (b <= 0) return '0 B/s';
            if (b < 1024) return Math.round(b) + ' B/s';
            if (b < 1024 ** 2) return (b / 1024).toFixed(1) + ' KB/s';
            return (b / (1024 ** 2)).toFixed(1) + ' MB/s';
        }

        // Initialize empty arrays for 60 seconds
        const MAX_DATA_POINTS = 60;
        let cpuData = Array(MAX_DATA_POINTS).fill(0);
        let memRamData = Array(MAX_DATA_POINTS).fill(0);
        let memZramData = Array(MAX_DATA_POINTS).fill(0);
        let memSwapData = Array(MAX_DATA_POINTS).fill(0);
        let gpuRcsData = Array(MAX_DATA_POINTS).fill(0);
        let gpuBcsData = Array(MAX_DATA_POINTS).fill(0);
        let gpuVcsData = Array(MAX_DATA_POINTS).fill(0);
        let gpuVecsData = Array(MAX_DATA_POINTS).fill(0);
        let netRxData = Array(MAX_DATA_POINTS).fill(0);
        let netTxData = Array(MAX_DATA_POINTS).fill(0);
        let diskReadData = Array(MAX_DATA_POINTS).fill(0);
        let diskWriteData = Array(MAX_DATA_POINTS).fill(0);
        let diskHistoryMap = {};
        let labels = Array.from({length: MAX_DATA_POINTS}, (_, i) => i);

        // Declare all Chart instances at top scope
        let cpuChart = null;
        let gpuChart = null;
        let memChart = null;
        let netChart = null;
        let diskChart = null;

        // CPU Chart
        Chart.defaults.maintainAspectRatio = false;
        const cpuChartCtx = document.getElementById('cpuChart').getContext('2d');
        cpuChart = new Chart(cpuChartCtx, {
            type: 'line',
            data: {
                labels: labels,
                datasets: [{
                    label: 'Usage %',
                    data: cpuData,
                    borderColor: '#3b82f6',
                    backgroundColor: 'rgba(59, 130, 246, 0.1)',
                    borderWidth: 2,
                    fill: true,
                    tension: 0.4,
                    pointRadius: 0
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                animation: false,
                scales: {
                    x: { display: false },
                    y: {
                        display: true,
                        position: 'right',
                        min: 0,
                        max: 100,
                        grid: {
                            color: 'rgba(255, 255, 255, 0.05)',
                            tickColor: 'rgba(255, 255, 255, 0.2)',
                            drawBorder: true
                        },
                        ticks: {
                            color: '#94a3b8',
                            font: { size: 9, weight: '600' },
                            stepSize: 50,
                            callback: function(val) { return val + '%'; }
                        }
                    }
                },
                plugins: { legend: { display: false }, tooltip: { enabled: false } }
            }
        });

        // Combined 4-Line GPU Chart
        const gpuChartCtx = document.getElementById('gpuChart').getContext('2d');
        gpuChart = new Chart(gpuChartCtx, {
            type: 'line',
            data: {
                labels: labels,
                datasets: [
                    { label: 'Render/3D %', data: gpuRcsData, borderColor: '#a855f7', borderWidth: 2, tension: 0.4, pointRadius: 0 },
                    { label: 'Blitter/2D %', data: gpuBcsData, borderColor: '#3b82f6', borderWidth: 2, tension: 0.4, pointRadius: 0 },
                    { label: 'Video Decode %', data: gpuVcsData, borderColor: '#06b6d4', borderWidth: 2, tension: 0.4, pointRadius: 0 },
                    { label: 'Video Encode %', data: gpuVecsData, borderColor: '#f59e0b', borderWidth: 2, tension: 0.4, pointRadius: 0 }
                ]
            },
            options: {
                responsive: true, maintainAspectRatio: false, animation: false,
                scales: {
                    x: { display: false },
                    y: {
                        display: true,
                        position: 'right',
                        min: 0,
                        max: 100,
                        grid: {
                            color: 'rgba(255, 255, 255, 0.05)',
                            tickColor: 'rgba(255, 255, 255, 0.2)',
                            drawBorder: true
                        },
                        ticks: {
                            color: '#94a3b8',
                            font: { size: 9, weight: '600' },
                            stepSize: 50,
                            callback: function(val) { return val + '%'; }
                        }
                    }
                },
                plugins: { legend: { display: false }, tooltip: { enabled: false } }
            }
        });

        // Combined 3-Line Memory Chart
        const memChartCtx = document.getElementById('memChart').getContext('2d');
        memChart = new Chart(memChartCtx, {
            type: 'line',
            data: {
                labels: labels,
                datasets: [
                    { label: 'RAM %', data: memRamData, borderColor: '#10b981', borderWidth: 2, tension: 0.4, pointRadius: 0 },
                    { label: 'ZRAM %', data: memZramData, borderColor: '#f59e0b', borderWidth: 2, tension: 0.4, pointRadius: 0 },
                    { label: 'SWAP %', data: memSwapData, borderColor: '#ef4444', borderWidth: 2, tension: 0.4, pointRadius: 0 }
                ]
            },
            options: {
                responsive: true, maintainAspectRatio: false, animation: false,
                scales: {
                    x: { display: false },
                    y: {
                        display: true,
                        position: 'right',
                        min: 0,
                        max: 100,
                        grid: {
                            color: 'rgba(255, 255, 255, 0.05)',
                            tickColor: 'rgba(255, 255, 255, 0.2)',
                            drawBorder: true
                        },
                        ticks: {
                            color: '#94a3b8',
                            font: { size: 9, weight: '600' },
                            stepSize: 50,
                            callback: function(val) { return val + '%'; }
                        }
                    }
                },
                plugins: { legend: { display: false }, tooltip: { enabled: false } }
            }
        });

        // Net Chart
        const netChartCtx = document.getElementById('netChart').getContext('2d');
        netChart = new Chart(netChartCtx, {
            type: 'line',
            data: {
                labels: labels,
                datasets: [
                    { data: netRxData, borderColor: '#06b6d4', borderWidth: 2, tension: 0.4, pointRadius: 0 },
                    { data: netTxData, borderColor: '#3b82f6', borderWidth: 2, tension: 0.4, pointRadius: 0 }
                ]
            },
            options: {
                responsive: true, maintainAspectRatio: false, animation: false,
                scales: {
                    x: { display: false },
                    y: {
                        display: true,
                        position: 'right',
                        min: 0,
                        grid: {
                            color: 'rgba(255, 255, 255, 0.05)',
                            tickColor: 'rgba(255, 255, 255, 0.2)',
                            drawBorder: true
                        },
                        ticks: {
                            color: '#94a3b8',
                            font: { size: 9, weight: '600' },
                            maxTicksLimit: 3,
                            callback: function(val) { return formatBytes(val) + '/s'; }
                        }
                    }
                },
                plugins: { legend: { display: false }, tooltip: { enabled: false } }
            }
        });

        // Disk Chart
        const diskChartCtx = document.getElementById('diskChart').getContext('2d');
        diskChart = new Chart(diskChartCtx, {
            type: 'line',
            data: {
                labels: labels,
                datasets: [
                    { data: diskReadData, borderColor: '#f59e0b', borderWidth: 2, tension: 0.4, pointRadius: 0 },
                    { data: diskWriteData, borderColor: '#ef4444', borderWidth: 2, tension: 0.4, pointRadius: 0 }
                ]
            },
            options: {
                responsive: true, maintainAspectRatio: false, animation: false,
                scales: {
                    x: { display: false },
                    y: {
                        display: true,
                        position: 'right',
                        min: 0,
                        grid: {
                            color: 'rgba(255, 255, 255, 0.05)',
                            tickColor: 'rgba(255, 255, 255, 0.2)',
                            drawBorder: true
                        },
                        ticks: {
                            color: '#94a3b8',
                            font: { size: 9, weight: '600' },
                            maxTicksLimit: 3,
                            callback: function(val) { return formatBytes(val) + '/s'; }
                        }
                    }
                },
                plugins: { legend: { display: false }, tooltip: { enabled: false } }
            }
        });

        function resyncChartFonts() {
            const zoomRatio = currentZoom / 16; // 16 = base default font size
            [cpuChart, gpuChart, memChart, netChart, diskChart].forEach(ch => {
                if (!ch) return;
                const card = ch.canvas.closest('.card');
                const fsBoost = (card && card.classList.contains('fullscreen')) ? 1.5 : 1;
                const newSize = Math.round(9 * zoomRatio * fsBoost);
                if (ch.options.scales.y && ch.options.scales.y.ticks) {
                    ch.options.scales.y.ticks.font.size = newSize;
                }
                ch.update('none');
            });
        }

        let _prevCores = '';
        let _prevTopCpu = '';
        let _prevTopMem = '';
        let _prevTopNet = '';
        let _prevTopGpu = '';
        let _prevTopDisk = '';

        let selectedGpuId = null;

        let _renderedOsCredits = null;

        function renderOsCredits(osType) {
            if (_renderedOsCredits === osType) return;
            _renderedOsCredits = osType;

            const pillsCont = document.getElementById('about-tech-pills');
            const contentCont = document.getElementById('about-dynamic-content');
            const frozenPillsCont = document.getElementById('frozen-tech-pills');
            const frozenContentCont = document.getElementById('frozen-dynamic-content');
            if (!pillsCont || !contentCont) return;

            if (osType === 'linux') {
                const pillsHtml = `
                    <span class="tech-pill">C++</span>
                    <span class="tech-pill">HTML</span>
                    <span class="tech-pill">CSS</span>
                    <span class="tech-pill">JavaScript</span>
                    <span class="tech-pill">GTK+ 3.0</span>
                    <span class="tech-pill">WebKit2GTK</span>
                `;
                pillsCont.innerHTML = pillsHtml;
                if (frozenPillsCont) frozenPillsCont.innerHTML = pillsHtml;

                contentCont.innerHTML = `
                    <div class="about-section">
                        <div class="about-section-heading">System Specifications</div>
                        <div class="about-spec-list">
                            <div class="about-spec-row">
                                <span class="about-spec-label">Core Runtime</span>
                                <span class="about-spec-value" style="color:var(--accent-blue);">Native ISO C++17</span>
                            </div>
                            <div class="about-spec-row">
                                <span class="about-spec-label">Interface Host</span>
                                <span class="about-spec-value">WebKit2GTK / Linux Display Server (X11/Wayland)</span>
                            </div>
                            <div class="about-spec-row">
                                <span class="about-spec-label">Telemetry Pipeline</span>
                                <span class="about-spec-value" style="color:var(--accent-green);">Linux Kernel Telemetry (/proc &amp; /sys)</span>
                            </div>
                            <div class="about-spec-row">
                                <span class="about-spec-label">Storage Diagnostics</span>
                                <span class="about-spec-value" style="color:var(--accent-orange);">Sysfs Block Engine / SMART Passthrough</span>
                            </div>
                            <div class="about-spec-row">
                                <span class="about-spec-label">Graphics Provider</span>
                                <span class="about-spec-value">Linux Direct Rendering Manager (DRM / KMS)</span>
                            </div>
                        </div>
                    </div>

                    <div class="about-section">
                        <div class="about-section-heading">Architecture Details</div>
                        <div class="about-spec-list">
                            <div class="about-spec-row">
                                <span class="about-spec-label">Telemetry Latency</span>
                                <span class="about-spec-value">500ms Non-Blocking Polling (Sub-millisecond Compute)</span>
                            </div>
                            <div class="about-spec-row">
                                <span class="about-spec-label">Subprocess Overhead</span>
                                <span class="about-spec-value">Minimal Subprocess Overhead (Kernel /sys &amp; /proc with smartctl/nvidia-smi passthrough)</span>
                            </div>
                            <div class="about-spec-row">
                                <span class="about-spec-label">Memory Topology</span>
                                <span class="about-spec-value">Physical RAM, ZRAM Compressed Engine &amp; SWAP Pool</span>
                            </div>
                            <div class="about-spec-row">
                                <span class="about-spec-label">Display Mode</span>
                                <span class="about-spec-value">Frameless GTK App-Paintable (Native Cairo Clipping)</span>
                            </div>
                        </div>
                    </div>

                    <div class="about-section">
                        <div class="about-section-heading">Execution Environment</div>
                        <div class="about-spec-list">
                            <div class="about-spec-row">
                                <span class="about-spec-label">Security &amp; Integrity</span>
                                <span class="about-spec-value">Standard User Mode Execution</span>
                            </div>
                            <div class="about-spec-row">
                                <span class="about-spec-label">Binary Footprint</span>
                                <span class="about-spec-value">Compiled Native Binary / Lightweight Footprint</span>
                            </div>
                            <div class="about-spec-row">
                                <span class="about-spec-label">Platform Target</span>
                                <span class="about-spec-value">Linux x86_64</span>
                            </div>
                        </div>
                    </div>

                    <div class="about-section">
                        <div class="about-section-heading">Source Repository</div>
                        <a class="github-link" href="https://github.com/rizkybayuu" target="_blank" rel="noopener noreferrer">
                            <svg class="github-icon" viewBox="0 0 24 24" aria-hidden="true">
                                // <path d="M12 0C5.37 0 0 5.37 0 12c0 5.31 3.435 9.795 8.205 11.385.6.105.825-.255.825-.57 0-.285-.015-1.23-.015-2.235-3.015.555-3.795-.735-4.035-1.41-.135-.345-.72-1.41-1.23-1.695-.42-.225-1.02-.78-.015-.795.945-.015 1.62.87 1.845 1.23 1.08 1.815 2.805 1.305 3.495.99.105-.78.42-1.305.765-1.605-2.67-.3-5.46-1.335-5.46-5.925 0-1.305.465-2.385 1.23-3.225-.12-.3-.54-1.53.12-3.18 0 0 1.005-.315 3.3 1.23.96-.27 1.98-.405 3-.405s2.04.135 3 .405c2.295-1.56 3.3-1.23 3.3-1.23.66 1.65.24 2.88.12 3.18.765.84 1.23 1.905 1.23 3.225 0 4.605-2.805 5.625-5.475 5.925.435.375.81 1.095.81 2.22 0 1.605-.015 2.895-.015 3.3 0 .315.225.69.825.57A12.02 12.02 0 0024 12c0-6.63-5.37-12-12-12z"/>
                            </svg>
                            github.com/rizkybayuu
                        </a>
                        <p class="closing-note">
                            Thank you for using this monitor application.<br>
                            Wish you all the best!
                        </p>
                    </div>
                `;
            } else if (osType === 'windows') {
                const pillsHtml = `
                    <span class="tech-pill">C++</span>
                    <span class="tech-pill">HTML</span>
                    <span class="tech-pill">CSS</span>
                    <span class="tech-pill">JavaScript</span>
                    <span class="tech-pill">Win32 API</span>
                    <span class="tech-pill">WebView2</span>
                `;
                pillsCont.innerHTML = pillsHtml;
                if (frozenPillsCont) frozenPillsCont.innerHTML = pillsHtml;

                contentCont.innerHTML = `
                    <div class="about-section">
                        <div class="about-section-heading">System Specifications</div>
                        <div class="about-spec-list">
                            <div class="about-spec-row">
                                <span class="about-spec-label">Core Runtime</span>
                                <span class="about-spec-value" style="color:var(--accent-blue);">Native ISO C++17</span>
                            </div>
                            <div class="about-spec-row">
                                <span class="about-spec-label">Interface Host</span>
                                <span class="about-spec-value">Microsoft WebView2 / Chromium Core</span>
                            </div>
                            <div class="about-spec-row">
                                <span class="about-spec-label">Telemetry Pipeline</span>
                                <span class="about-spec-value" style="color:var(--accent-green);">Win32 PDH / ETW Kernel Trace</span>
                            </div>
                            <div class="about-spec-row">
                                <span class="about-spec-label">Storage Diagnostics</span>
                                <span class="about-spec-value" style="color:var(--accent-orange);">ATA Passthrough / SCSI-SAT SMART</span>
                            </div>
                            <div class="about-spec-row">
                                <span class="about-spec-label">Graphics Provider</span>
                                <span class="about-spec-value">DXGI 1.4 Dynamic Composition</span>
                            </div>
                        </div>
                    </div>

                    <div class="about-section">
                        <div class="about-section-heading">Architecture Details</div>
                        <div class="about-spec-list">
                            <div class="about-spec-row">
                                <span class="about-spec-label">Telemetry Latency</span>
                                <span class="about-spec-value">500ms Non-Blocking Polling (Sub-millisecond Compute)</span>
                            </div>
                            <div class="about-spec-row">
                                <span class="about-spec-label">Subprocess Overhead</span>
                                <span class="about-spec-value">Zero WMIC / Zero CMD spawns</span>
                            </div>
                            <div class="about-spec-row">
                                <span class="about-spec-label">Memory Topology</span>
                                <span class="about-spec-value">Physical RAM, Smart Cache, Dedicated &amp; Shared VRAM</span>
                            </div>
                            <div class="about-spec-row">
                                <span class="about-spec-label">Display Mode</span>
                                <span class="about-spec-value">Frameless DWM Composition</span>
                            </div>
                        </div>
                    </div>

                    <div class="about-section">
                        <div class="about-section-heading">Execution Environment</div>
                        <div class="about-spec-list">
                            <div class="about-spec-row">
                                <span class="about-spec-label">Security &amp; Integrity</span>
                                <span class="about-spec-value">Standard User Privilege (No Elevation Required)</span>
                            </div>
                            <div class="about-spec-row">
                                <span class="about-spec-label">Binary Footprint</span>
                                <span class="about-spec-value">Single Executable / Portable Deployment</span>
                            </div>
                            <div class="about-spec-row">
                                <span class="about-spec-label">Platform Target</span>
                                <span class="about-spec-value">Windows 10 / 11 (x86_64 Architecture)</span>
                            </div>
                        </div>
                    </div>

                    <div class="about-section">
                        <div class="about-section-heading">Source Repository</div>
                        <a class="github-link" href="https://github.com/rizkybayuu" target="_blank" rel="noopener noreferrer">
                            <svg class="github-icon" viewBox="0 0 24 24" aria-hidden="true">
                                <path d="M12 0C5.37 0 0 5.37 0 12c0 5.31 3.435 9.795 8.205 11.385.6.105.825-.255.825-.57 0-.285-.015-1.23-.015-2.235-3.015.555-3.795-.735-4.035-1.41-.135-.345-.72-1.41-1.23-1.695-.42-.225-1.02-.78-.015-.795.945-.015 1.62.87 1.845 1.23 1.08 1.815 2.805 1.305 3.495.99.105-.78.42-1.305.765-1.605-2.67-.3-5.46-1.335-5.46-5.925 0-1.305.465-2.385 1.23-3.225-.12-.3-.54-1.53.12-3.18 0 0 1.005-.315 3.3 1.23.96-.27 1.98-.405 3-.405s2.04.135 3 .405c2.295-1.56 3.3-1.23 3.3-1.23.66 1.65.24 2.88.12 3.18.765.84 1.23 1.905 1.23 3.225 0 4.605-2.805 5.625-5.475 5.925.435.375.81 1.095.81 2.22 0 1.605-.015 2.895-.015 3.3 0 .315.225.69.825.57A12.02 12.02 0 0024 12c0-6.63-5.37-12-12-12z"/>
                            </svg>
                            github.com/rizkybayuu
                        </a>
                        <p class="closing-note">
                            Thank you for using this monitor application.<br>
                            Wish you all the best!
                        </p>
                    </div>
                `;
            } else {
                // Keduanya invalid -> kosongkan total
                pillsCont.innerHTML = '';
                contentCont.innerHTML = '';
                syncFrozenCreditClone();
            }
        }

        async function fetchStats() {
            try {
                const response = await fetch('/api/stats?win=' + windowId);
                const data = await response.json();

                // Utamakan data credits dari backend C++; gunakan renderOsCredits hanya sebagai fallback
                if (data.credits && data.credits.valid) {
                    updateAboutPanel(data.credits);
                } else {
                    renderOsCredits(data.os_type);
                }

                // Update Hardware Names Tooltips
                if (data.hardware) {
                    const totalCores = (data.cpu && data.cpu.cores) ? data.cpu.cores.length : 0;
                    const cpuModel = data.cpu_model || data.hardware.cpu_model || "Processor";
                    const gpuModel = data.gpu_model || data.hardware.gpu_model || "Graphics Processor";

                    window.hardwareTooltips.cpu = `<strong>Processor Model:</strong><br>${cpuModel}<br><strong>Detected Logic Cores:</strong> ${totalCores > 0 ? totalCores + ' Threads' : 'Detecting...'}<br><strong>Architecture:</strong> Adaptive Multiprocessing System`;

                    window.hardwareTooltips.gpu = `<strong>Graphics Subsystem:</strong><br>${gpuModel}<br><strong>Hardware Engines:</strong> Render/3D (RCS), Video Decode (VCS), Blitter (BCS), Video Enhance (VECS)<br><strong>Engine Clock:</strong> ${data.gpu ? data.gpu.freq : 0} MHz`;
                }
                if(data.details) {
                    const ramType = Array.isArray(data.details.ram_type) ? data.details.ram_type.join('<br>') : (data.details.ram_type || "Slot 1: 8 GiB DDR4 (3200 MT/s)<br>Slot 2: 8 GiB DDR4 (3200 MT/s)");
                    window.hardwareTooltips.ram = `<strong>Primary Volatile Memory (RAM):</strong><br>${ramType}<br><strong>Architecture:</strong> 16.0 GB Dual-Channel SODIMM DDR4-3200<br><strong>Hierarchy:</strong> High-Throughput Interleaved Physical Memory`;
                    window.hardwareTooltips.zram = `<strong>ZRAM Compressed Swap Engine:</strong><br>${data.details.zram_info || '8.0 GB zstd compressed in-RAM swap pool'}<br><strong>Compression:</strong> zstd parallel block compression engine<br><strong>Access Latency:</strong> Zero block device storage latency`;
                    window.hardwareTooltips.swap = `<strong>System Swap Subsystem:</strong><br>${data.details.swap_info || 'Active ZRAM Swap Pool (Priority 100)'}<br><strong>Paging Strategy:</strong> Zero disk latency, high-throughput memory exchange`;
                    window.hardwareTooltips.ssd = `<strong>Storage Devices & Drives:</strong><br>${data.details.ssd_model || 'SanDisk Portable SSD (931.5G) + NVMe (238.5G)'}<br><strong>Health Telemetry:</strong> Real-time TBW (Total Bytes Written) & SMART status`;
                    window.hardwareTooltips.network = data.details.network_type || 'Intel Wi-Fi + Realtek Gigabit Ethernet';
                    window.hardwareTooltips.battery = `<strong>Power & Battery Management:</strong><br>${data.details.battery_tech || 'SR Real Battery'}`;
                    let tempDisplay = (data.sensors && data.sensors.temp > 0) ? `${data.sensors.temp}°C` : "N/A (Not Exposed by ACPI)";
                    let tempNote = (data.sensors && data.sensors.temp > 0) ? "Real-time ACPI Thermal Zone monitoring" : "ACPI Thermal Zone inactive / Ring-0 driver required for CPU MSR";
                    window.hardwareTooltips.temperature = `<strong>🌡️ Thermal Management:</strong><br>Current CPU Package Temp: <strong>${tempDisplay}</strong><br><strong>Sensors:</strong> ${tempNote}`;
                    
                    if(data.details.raw_cpu && document.getElementById('cpu-details').textContent !== data.details.raw_cpu) document.getElementById('cpu-details').textContent = data.details.raw_cpu;
                    if(data.details.raw_gpu && document.getElementById('gpu-details').textContent !== data.details.raw_gpu) document.getElementById('gpu-details').textContent = data.details.raw_gpu;
                    if(data.details.raw_ram && document.getElementById('ram-details').textContent !== data.details.raw_ram) document.getElementById('ram-details').textContent = data.details.raw_ram;
                    if(data.details.raw_net && document.getElementById('net-details').textContent !== data.details.raw_net) document.getElementById('net-details').textContent = data.details.raw_net;
                    if(data.details.raw_disk && document.getElementById('disk-details') && document.getElementById('disk-details').textContent.startsWith('Loading')) document.getElementById('disk-details').textContent = data.details.raw_disk;
                }

                // 1. CPU Section
                const cpuUsage = data.cpu ? data.cpu.total_usage : 0;
                document.getElementById('cpu-total').textContent = `${cpuUsage}%`;
                document.getElementById('cpu-bar').style.width = `${cpuUsage}%`;
                
                cpuData.push(cpuUsage);
                cpuData.shift();
                cpuChart.update();
                
                if (data.cpu && data.cpu.cores) {
                    const coresKey = JSON.stringify(data.cpu.cores) + JSON.stringify(data.cpu.freqs) + JSON.stringify(data.cpu.core_tags);
                    if (coresKey !== _prevCores) {
                        _prevCores = coresKey;
                        const coresContainer = document.getElementById('cpu-cores');
                        coresContainer.innerHTML = '';

                        data.cpu.cores.forEach((usage, i) => {
                            const freq = data.cpu.freqs[i] || 0;
                            const coreTag = (data.cpu.core_tags && data.cpu.core_tags[i]) ? data.cpu.core_tags[i] : `C${i}`;
                            const coreType = (data.cpu.core_types && data.cpu.core_types[i]) ? data.cpu.core_types[i] : 'p-core';

                            let coreClass = 'core-box p-core';
                            let coreColor = 'var(--accent-blue)';

                            if (coreType === 'e-core') {
                                coreClass = 'core-box e-core';
                                coreColor = 'var(--accent-cyan)';
                            } else if (coreType === 'lp-core') {
                                coreClass = 'core-box e-core';
                                coreColor = 'var(--accent-green)';
                            }

                            coresContainer.innerHTML += `
                                <div class="${coreClass}">
                                    <div class="core-name" style="font-weight:700; color:${coreColor}">${coreTag}</div>
                                    <div class="core-val" style="color: ${usage > 80 ? 'var(--accent-red)' : coreColor}">${usage}%</div>
                                    <div class="core-freq">${freq}MHz</div>
                                </div>
                            `;
                        });
                    }
                }

                // Top CPU Processes
                if (data.processes && data.processes.cpu) {
                    const topCpuKey = JSON.stringify(data.processes.cpu);
                    if (topCpuKey !== _prevTopCpu) {
                        _prevTopCpu = topCpuKey;
                        const topCpuCont = document.getElementById('top-cpu');
                        topCpuCont.innerHTML = data.processes.cpu.map((p, idx) => {
                            const pctVal = parseFloat(p.val) || 0;
                            return `
                                <div class="list-item" style="display:flex; align-items:center; gap:6px;">
                                    <span style="color: var(--accent-blue); font-weight: 700; flex-shrink: 0; min-width: 1.4rem;">${idx + 1}.</span>
                                    <span class="list-name" style="flex:1; min-width:0;" title="${p.name}">${p.name}</span>
                                    <div style="flex-shrink:0; width:3rem; height:0.4vh; background:rgba(255,255,255,0.08); border-radius:4px; overflow:hidden;">
                                        <div style="width:${Math.min(100, pctVal)}%; height:100%; background:var(--accent-blue); border-radius:4px;"></div>
                                    </div>
                                    <span class="list-val" style="color: var(--accent-blue); font-weight: 700; flex-shrink: 0; min-width: 2.2rem; text-align:right;">${pctVal.toFixed(1)}%</span>
                                </div>
                            `;
                        }).join('');
                    }
                }

                // 2. GPU Section (Fixed GPU VRAM Percentage Progress Bar)
                const gUsage = (data.gpu && data.gpu.usage) ? data.gpu.usage : {};
                const rcs = Number(gUsage.rcs) || 0;
                const bcs = Number(gUsage.bcs) || 0;
                const vcs = Number(gUsage.vcs) || 0;
                const vecs = Number(gUsage.vecs) || 0;
                
                document.getElementById('gpu-rcs-val').textContent = `${rcs.toFixed(1)}%`;
                document.getElementById('gpu-rcs-bar').style.width = `${Math.min(100, rcs)}%`;
                document.getElementById('gpu-bcs-val').textContent = `${bcs.toFixed(1)}%`;
                document.getElementById('gpu-bcs-bar').style.width = `${Math.min(100, bcs)}%`;
                document.getElementById('gpu-vcs-val').textContent = `${vcs.toFixed(1)}%`;
                document.getElementById('gpu-vcs-bar').style.width = `${Math.min(100, vcs)}%`;
                document.getElementById('gpu-vecs-val').textContent = `${vecs.toFixed(1)}%`;
                document.getElementById('gpu-vecs-bar').style.width = `${Math.min(100, vecs)}%`;

                gpuRcsData.push(rcs); gpuRcsData.shift();
                gpuBcsData.push(bcs); gpuBcsData.shift();
                gpuVcsData.push(vcs); gpuVcsData.shift();
                gpuVecsData.push(vecs); gpuVecsData.shift();
                if (gpuChart) gpuChart.update();

                if (data.gpus && data.gpus.length > 0) {
                    updateGpuSelector(data.gpus);
                } else {
                    updateGpuSelector([{ id: 'igpu', name: data.hardware ? data.hardware.gpu_model : 'Intel Iris Xe', is_egpu: false }]);
                }

                // GPU Processes List Update (Fix Realtime Hang)
                let gProcs = (data.processes && data.processes.gpu) ? data.processes.gpu : [];
                const topGpuKey = gProcs.map(p => p.name + '_' + p.total.toFixed(1)).join(',');

                if (topGpuKey !== _prevTopGpu) {
                    _prevTopGpu = topGpuKey;
                    const topGpuCont = document.getElementById('top-gpu');
                    if (topGpuCont) {
                        if (gProcs && gProcs.length > 0) {
                            const prevScroll = topGpuCont.scrollTop;
                            topGpuCont.innerHTML = gProcs.map((p, idx) => `
                                <div class="list-item" style="display:flex; align-items:center; gap:6px;">
                                    <span style="color: var(--accent-purple); font-weight: 700; flex-shrink: 0; min-width: 1.4rem;">${idx + 1}.</span>
                                    <span class="list-name" style="flex:1; min-width:0;" title="${p.name}">${p.name}</span>
                                    <span class="list-val" style="font-weight: 500; flex-shrink: 0; display:flex; gap:6px; align-items:center; justify-content:flex-end;">
                                        <span style="color: var(--accent-purple);" title="Graphics/3D">${p.rcs.toFixed(1)}%</span>
                                        <span style="color: var(--accent-blue);" title="Blitter/2D">${p.bcs.toFixed(1)}%</span>
                                        <span style="color: var(--accent-cyan);" title="Video Decode">${p.vcs.toFixed(1)}%</span>
                                        <span style="color: var(--accent-orange);" title="Video Enhance">${p.vecs.toFixed(1)}%</span>
                                    </span>
                                </div>
                            `).join('');
                            topGpuCont.scrollTop = prevScroll;
                        } else {
                            topGpuCont.innerHTML = `<div class="list-item" style="justify-content:center; color:var(--text-muted); font-style:italic;">No Active GPU Processes</div>`;
                        }
                    }
                }

                // 3. RAM & Memory Hierarchy Progress Bars
                const ramUsed = data.ram ? data.ram.used : 0;
                const ramTotal = data.ram ? data.ram.total : 16 * (1024**3);
                const activeGpu = (data.gpus && data.gpus.length > 0) ? (data.gpus.find(g => String(g.id) === String(selectedGpuId)) || data.gpus[0]) : null;
                
                // --- PERBAIKAN VRAM DEDICATED (Mencegah NaNs / Error 0%) ---
                let dedicatedVram = 0;
                let dedicatedVramTotal = 0;
                if (data.vram) {
                    dedicatedVram = data.vram.dedicated_used || 0;
                    dedicatedVramTotal = data.vram.dedicated_total || 0;
                } else if (activeGpu) {
                    dedicatedVramTotal = (activeGpu.dedicated_vram_gb || 0) * (1024 ** 3);
                    dedicatedVram = dedicatedVramTotal * ((activeGpu.usage_pct || 0) / 100);
                }

                const swapUsed = data.swap ? data.swap.used : 0;
                const swapTotal = data.swap ? data.swap.total : 0;

                const combinedUsed = ramUsed + dedicatedVram + swapUsed;
                const combinedTotal = ramTotal + dedicatedVramTotal + swapTotal;
                const combinedPct = combinedTotal > 0 ? (combinedUsed / combinedTotal) * 100 : 0;

                const ramTextEl = document.getElementById('ram-text');
                if (ramTextEl) ramTextEl.innerHTML = `<span style="color:var(--accent-green); font-weight:700;">${formatBytes(combinedUsed)}</span> / ${formatBytes(combinedTotal)}`;
                const ramBarEl = document.getElementById('ram-bar');
                if (ramBarEl) ramBarEl.style.width = `${combinedPct}%`;

                // Update 6 Dynamic Memory Rows & Progress Bars
                const memShared = (data.vram && data.vram.shared_total) ? data.vram.shared_total : (activeGpu ? activeGpu.shared_vram_gb * (1024 ** 3) : 0);
                const zramUsed = data.zram ? data.zram.used : 0;
                const zramTotal = data.zram ? data.zram.total : 0;

                // Helper pembersih: Ubah '0', kosong, atau undefined jadi 'N/A'
                const sanitizeNa = (val) => (!val || val === '0' || val === '0 B' || val === '0 / 0') ? 'N/A' : val;

                // 1. Smart Cache
                const elCacheVal = document.getElementById('mem-val-cache');
                const rowCache = document.getElementById('mem-row-cache');
                const barCache = document.getElementById('mem-bar-cache');
                if (elCacheVal) {
                    let cacheStr = (data.smart_cache && data.smart_cache.display_str) ? data.smart_cache.display_str :
                                    (data.cpu_cache && data.cpu_cache.smart_cache_total ? formatBytes(data.cpu_cache.smart_cache_total) : 'N/A');
                    cacheStr = sanitizeNa(cacheStr);
                    elCacheVal.textContent = cacheStr;

                    const hasCache = cacheStr !== 'N/A' && cacheStr !== 'Detecting...';
                    if (rowCache) rowCache.style.opacity = hasCache ? '1' : '0.35';
                    if (barCache) barCache.style.width = hasCache ? '100%' : '0%';
                }

                // 2. Dedicated VRAM
                const rowVram = document.getElementById('mem-row-vram');
                const elVramVal = document.getElementById('mem-val-vram');
                const elVramBar = document.getElementById('mem-bar-vram');

                let vramStr = (data.vram && data.vram.dedicated_str) ? data.vram.dedicated_str :
                                (dedicatedVramTotal > 0 ? `${formatBytes(dedicatedVram)} / ${formatBytes(dedicatedVramTotal)}` : 'N/A');
                vramStr = sanitizeNa(vramStr);

                const hasVram = (dedicatedVramTotal > 0 && vramStr !== 'N/A');
                if (rowVram) rowVram.style.opacity = hasVram ? '1' : '0.35';
                if (elVramVal) elVramVal.textContent = vramStr;
                if (elVramBar) {
                    const vramPct = dedicatedVramTotal > 0 ? (dedicatedVram / dedicatedVramTotal) * 100 : 0;
                    elVramBar.style.width = hasVram ? `${Math.min(100, Math.max(0, vramPct))}%` : '0%';
                }

                // 3. Physical RAM
                const elRamVal = document.getElementById('mem-val-ram');
                const elRamBar = document.getElementById('mem-bar-ram');
                const ramStr = (data.ram && data.ram.display_str) ? data.ram.display_str : `${formatBytes(ramUsed)} / ${formatBytes(ramTotal)}`;
                if (elRamVal) elRamVal.textContent = ramStr;
                if (elRamBar) elRamBar.style.width = ramTotal > 0 ? `${(ramUsed / ramTotal) * 100}%` : '0%';

                // 4. Shared VRAM
                const rowShared = document.getElementById('mem-row-shared');
                const elSharedVal = document.getElementById('mem-val-shared');
                const elSharedBar = document.getElementById('mem-bar-shared');

                let sharedStr = (data.vram && data.vram.shared_str) ? data.vram.shared_str :
                                (memShared > 0 ? formatBytes(memShared) : 'N/A');
                sharedStr = sanitizeNa(sharedStr);

                const hasShared = (memShared > 0 && sharedStr !== 'N/A');
                if (rowShared) rowShared.style.opacity = hasShared ? '1' : '0.35';
                if (elSharedVal) elSharedVal.textContent = sharedStr;
                if (elSharedBar) elSharedBar.style.width = hasShared ? '100%' : '0%';

                // 5. ZRAM Telemetry
                const rowZram = document.getElementById('mem-row-zram');
                const elZramVal = document.getElementById('mem-val-zram');
                const elZramBar = document.getElementById('mem-bar-zram');

                if (data.os_type === "windows") {
                    if (rowZram) rowZram.style.opacity = '0.35';
                    if (elZramVal) elZramVal.textContent = "N/A (Linux Only)";
                    if (elZramBar) elZramBar.style.width = '0%';
                } else if (!zramTotal || zramTotal === 0) {
                    if (rowZram) rowZram.style.opacity = '0.35';
                    if (elZramVal) elZramVal.textContent = "N/A (Not Set)";
                    if (elZramBar) elZramBar.style.width = '0%';
                } else {
                    const zramStr = (data.zram && data.zram.display_str) ? data.zram.display_str : `${formatBytes(zramUsed)} / ${formatBytes(zramTotal)}`;
                    if (rowZram) rowZram.style.opacity = '1';
                    if (elZramVal) elZramVal.textContent = zramStr;
                    if (elZramBar) elZramBar.style.width = `${(zramUsed / zramTotal) * 100}%`;
                }

                // 6. SWAP / Pagefile
                const rowSwap = document.getElementById('mem-row-swap');
                const elSwapLabel = document.getElementById('mem-label-swap');
                const elSwapVal = document.getElementById('mem-val-swap');
                const elSwapBar = document.getElementById('mem-bar-swap');

                if (elSwapLabel) {
                    elSwapLabel.textContent = data.os_type === "windows" ? "Pagefile (Virtual Memory)" : "Disk SWAP Space";
                }

                let swapStr = (data.swap && data.swap.display_str) ? data.swap.display_str :
                                (swapTotal > 0 ? `${formatBytes(swapUsed)} / ${formatBytes(swapTotal)}` : 'N/A (Not Set)');
                swapStr = sanitizeNa(swapStr);

                const hasSwap = (swapTotal > 0 && swapStr !== 'N/A');
                if (rowSwap) rowSwap.style.opacity = hasSwap ? '1' : '0.35';
                if (elSwapVal) elSwapVal.textContent = swapStr;
                if (elSwapBar) elSwapBar.style.width = hasSwap ? `${(swapUsed / swapTotal) * 100}%` : '0%';

                // Update Multi-Line Memory Chart
                memRamData.push((ramUsed / ramTotal) * 100); memRamData.shift();
                memZramData.push(zramTotal > 0 ? (zramUsed / zramTotal) * 100 : 0); memZramData.shift();
                memSwapData.push(swapTotal > 0 ? (swapUsed / swapTotal) * 100 : 0); memSwapData.shift();
                if (memChart) memChart.update();

                // Memory Processes List
                if (data.processes && data.processes.mem) {
                    const topMemKey = JSON.stringify(data.processes.mem);
                    if (topMemKey !== _prevTopMem) {
                        _prevTopMem = topMemKey;
                        const topMemCont = document.getElementById('top-mem');
                        topMemCont.innerHTML = data.processes.mem.map((p, idx) => `
                            <div class="list-item" style="display:flex; align-items:center; gap:6px;">
                                <span style="color: var(--accent-green); font-weight: 700; flex-shrink: 0; min-width: 1.4rem;">${idx + 1}.</span>
                                <span class="list-name" style="flex:1; min-width:0;">${p.name}</span>
                                <span class="list-val" style="color: var(--accent-green); font-weight: 700; flex-shrink: 0;">${p.val}</span>
                            </div>
                        `).join('');
                    }
                }

                // 4. Network Section
                const rxRate = data.network ? data.network.rx_rate : 0;
                const txRate = data.network ? data.network.tx_rate : 0;

                document.getElementById('net-rx').textContent = formatSpeed(rxRate);
                document.getElementById('net-tx').textContent = formatSpeed(txRate);
                
                netRxData.push(rxRate); netRxData.shift();
                netTxData.push(txRate); netTxData.shift();
                if (netChart) netChart.update();

                // Network Processes List
                if (data.processes && data.processes.net) {
                    const topNetKey = JSON.stringify(data.processes.net);
                    if (topNetKey !== _prevTopNet) {
                        _prevTopNet = topNetKey;
                        const topNetCont = document.getElementById('top-net');
                        if (data.processes.net.length > 0) {
                            topNetCont.innerHTML = data.processes.net.map((p, idx) => `
                                <div class="list-item" style="display:flex; align-items:center; gap:6px;">
                                    <span style="color: var(--accent-cyan); font-weight: 700; flex-shrink: 0; min-width: 1.4rem;">${idx + 1}.</span>
                                    <span class="list-name" style="flex:1; min-width:0;" title="${p.name}">${p.name}</span>
                                    <span class="list-val" style="font-weight: 500; flex-shrink: 0; display:flex; align-items:center; gap:6px;">
                                        <span style="color: var(--accent-cyan); font-weight: 500; min-width: 3.2rem; text-align: right;">▼ ${p.down}</span>
                                        <span style="color: var(--accent-blue); font-weight: 500; min-width: 3.2rem; text-align: right;">▲ ${p.up}</span>
                                    </span>
                                </div>
                            `).join('');
                        } else {
                            topNetCont.innerHTML = `<div class="list-item" style="justify-content:center; color:var(--text-muted); font-style:italic;">No Active Traffic / Listening...</div>`;
                        }
                    }
                }

                // 5. Disk Section
                // SESUDAH — tambahkan loop terpisah yang push history ke SEMUA disk tiap tick,
                // terlepas dari mana yang lagi ditampilkan di layar.
                if (data.disk && data.disk.disks && data.disk.disks.length > 0) {
                    data.disk.disks.forEach(d => {
                        const r = Math.max(0, Number(d.read_rate) || 0);
                        const w = Math.max(0, Number(d.write_rate) || 0);
                        const id = d.id || 'default';
                        if (!diskHistoryMap[id]) {
                            diskHistoryMap[id] = { read: Array(MAX_DATA_POINTS).fill(0), write: Array(MAX_DATA_POINTS).fill(0) };
                        }
                        diskHistoryMap[id].read.push(r);
                        diskHistoryMap[id].read.shift();
                        diskHistoryMap[id].write.push(w);
                        diskHistoryMap[id].write.shift();
                    });

                    updateDiskSelector(data.disk.disks, data.disk); // tetap panggil ini untuk render disk yg dipilih
                }

                // Disk Processes List
                let activeDev = selectedDiskId || (data.disk && data.disk.disks && data.disk.disks[0] ? data.disk.disks[0].id : 'sda');
                let dProcs = (data.processes && data.processes.disk_per_dev && data.processes.disk_per_dev[activeDev])
                    ? data.processes.disk_per_dev[activeDev]
                    : ((data.processes && data.processes.disk) ? data.processes.disk : []);
                const topDiskKey = dProcs.map(p => p.name + '_' + p.read + '_' + p.write).join(',') + '_' + activeDev;
                if (topDiskKey !== _prevTopDisk) {
                    _prevTopDisk = topDiskKey;
                    const topDiskCont = document.getElementById('top-disk');
                    if (topDiskCont) {
                        if (dProcs && dProcs.length > 0) {
                            topDiskCont.innerHTML = dProcs.map((p, idx) => `
                                <div class="list-item" style="display:flex; align-items:center; gap:6px;">
                                    <span style="color: var(--accent-orange); font-weight: 700; flex-shrink: 0; min-width: 1.4rem;">${idx + 1}.</span>
                                    <span class="list-name" style="flex:1; min-width:0;" title="${p.name}">${p.name}</span>
                                    <span class="list-val" style="font-weight: 500; flex-shrink: 0; display:flex; gap:6px;">
                                        <span style="color: var(--accent-orange); font-weight: 500;">R: ${p.read}</span>
                                        <span style="color: var(--accent-red); font-weight: 500;">W: ${p.write}</span>
                                    </span>
                                </div>
                            `).join('');
                        } else {
                            topDiskCont.innerHTML = `<div class="list-item" style="justify-content:center; color:var(--text-muted); font-style:italic;">No Active Disk I/O on ${activeDev}</div>`;
                        }
                    }
                }

				if (data.sensors) {
					const tempEl = document.getElementById('temp-val');
					if (tempEl) {
						tempEl.textContent = (data.sensors.temp && data.sensors.temp > 0) ? `${data.sensors.temp}°C` : 'N/A';
					}
					const battEl = document.getElementById('batt-val');
					if (battEl) {
						battEl.textContent = (data.sensors.battery !== undefined && data.sensors.battery >= 0) ? `${data.sensors.battery}%` : 'AC';
					}
				}

            } catch (err) {
                console.error("Error:", err);
            }
        }

        let selectedDiskId = null;
        let lastDiskData = null;

        let lastGpuList = [];

        function handleGpuScroll(e) {
            e.preventDefault();
            e.stopPropagation();
            if (!lastGpuList || lastGpuList.length <= 1) return;

            let currIdx = lastGpuList.findIndex(g => String(g.id) === String(selectedGpuId));
            if (currIdx === -1) currIdx = 0;
            if (e.deltaY > 0) {
                currIdx = (currIdx + 1) % lastGpuList.length;
            } else {
                currIdx = (currIdx - 1 + lastGpuList.length) % lastGpuList.length;
            }
            selectCustomGpu(lastGpuList[currIdx].id, e);
        }

        function updateGpuSelector(gpusList) {
            let menu = document.getElementById('custom-gpu-options');
            if (!menu) {
                menu = document.createElement('div');
                menu.id = 'custom-gpu-options';
                menu.className = 'custom-options-menu';
                menu.style.webkitAppRegion = 'no-drag';
                document.getElementById('app-window').appendChild(menu);
            }

            const labelTextEl = document.getElementById('gpu-label-text');

            // 100% Pure, Dynamic, and Adaptive Linux Backend Telemetry
            const list = (gpusList && gpusList.length > 0) ? gpusList : [{ id: 0, name: 'Graphics Processor', is_egpu: false }];
            lastGpuList = list;

            const savedGpu = safeStorage.getItem(`rizkyby_${windowId}_selected_gpu`) || safeStorage.getItem('rizkyby_selected_gpu');
            if (!selectedGpuId || !list.some(g => String(g.id) === String(selectedGpuId))) {
                const iGpu = list.find(g => g.is_egpu === false || g.is_egpu === "false");
                if (iGpu) {
                    selectedGpuId = iGpu.id; // Kunci mutlak ke iGPU
                } else {
                    selectedGpuId = list[0].id;
                }
            }

            const curGpu = list.find(g => String(g.id) === String(selectedGpuId)) || list[0];
            if (labelTextEl && curGpu) {
                const icon = curGpu.is_egpu ? '🚀' : '🎮';
                labelTextEl.textContent = `${icon} ${curGpu.name}`;
            }

            if (menu) {
                menu.innerHTML = list.map(g => `
                    <div class="custom-option-item ${String(g.id) === String(selectedGpuId) ? 'selected' : ''}" onclick="selectCustomGpu('${g.id}', event)">
                        ${g.is_egpu ? '🚀' : '🎮'} ${g.name} ${g.is_egpu ? '[eGPU]' : '[iGPU]'}
                    </div>
                `).join('');
            }

            // Dynamic Update of the 4-Engine Progress Bar Names Based on the Active GPU
            const e1 = document.getElementById('gpu-engine-1-name');
            const e2 = document.getElementById('gpu-engine-2-name');
            const e3 = document.getElementById('gpu-engine-3-name');
            const e4 = document.getElementById('gpu-engine-4-name');
            if (curGpu && curGpu.engines) {
                if (e1) e1.textContent = curGpu.engines[0] || 'Render/3D';
                if (e2) e2.textContent = curGpu.engines[1] || 'Blitter/2D';
                if (e3) e3.textContent = curGpu.engines[2] || 'Video Decode';
                if (e4) e4.textContent = curGpu.engines[3] || 'Video Enhance';
            } else if (curGpu && curGpu.is_egpu) {
                if (e1) e1.textContent = '3D/Compute';
                if (e2) e2.textContent = 'Copy Engine';
                if (e3) e3.textContent = 'NVDEC Video';
                if (e4) e4.textContent = 'NVENC Video';
            } else {
                if (e1) e1.textContent = 'Render/3D';
                if (e2) e2.textContent = 'Blitter/2D';
                if (e3) e3.textContent = 'Video Decode';
                if (e4) e4.textContent = 'Video Enhance';
            }

            // Render GPU Cores Grid (100% Identik dengan CPU Cores)
            const egpuCoresCont = document.getElementById('egpu-cores');
            if (egpuCoresCont && curGpu) {
                if (curGpu.is_egpu && curGpu.cores && curGpu.cores.length > 0 && curGpu.cores[0].val !== "Unified") {
                    // eGPU / Discrete GPU (Multi Engine Clusters)
                    egpuCoresCont.innerHTML = curGpu.cores.map(c => `
                        <div class="core-box p-core" style="border-color: var(--accent-purple); background: rgba(168, 85, 247, 0.06);">
                            <div class="core-name" style="font-weight: 700; color: var(--accent-purple);">${c.name}</div>
                            <div class="core-val" style="color: var(--accent-purple);">${c.val}</div>
                            <div class="core-freq">${c.sub || 'Engine'}</div>
                        </div>
                    `).join('');
                } else {
                    // iGPU (Unified Block dengan Class dan Hirarki Font Identik CPU Cores)
                    egpuCoresCont.innerHTML = `
                        <div class="core-box p-core" style="grid-column: 1 / -1; border-color: var(--accent-purple); background: rgba(168, 85, 247, 0.06);">
                            <div class="core-name" style="font-weight: 700; color: var(--accent-purple);">iGPU</div>
                            <div class="core-val" style="color: var(--accent-purple);">Unified</div>
                            <div class="core-freq">Execution Units</div>
                        </div>
                    `;
                }
            }
        }

        function toggleGpuMenu(e) {
            if (e && typeof e.stopPropagation === 'function') e.stopPropagation();
            let menu = document.getElementById('custom-gpu-options');
            const box = document.getElementById('custom-gpu-select');
            if (!menu || !box) return;
            const isShowing = menu.classList.contains('show');
            document.querySelectorAll('.custom-options-menu').forEach(m => m.classList.remove('show'));
            if (!isShowing) {
                menu.classList.add('show');
                const rect = box.getBoundingClientRect();
                menu.style.position = 'fixed';
                menu.style.top = (rect.bottom + 4) + 'px';
                
                menu.style.left = rect.left + 'px';
                menu.style.right = 'auto';
                menu.style.maxWidth = '90vw';
                
                const menuRect = menu.getBoundingClientRect();
                if (menuRect.right > window.innerWidth) {
                    menu.style.left = Math.max(10, window.innerWidth - menuRect.width - 10) + 'px';
                }
                
                menu.style.zIndex = '9990';
                menu.style.pointerEvents = 'auto';
            }
        }

        function selectCustomGpu(id, e) {
            if (e && typeof e.stopPropagation === 'function') e.stopPropagation();
            const menu = document.getElementById('custom-gpu-options');
            if (menu) menu.classList.remove('show');
            selectedGpuId = id;
            safeStorage.setItem(`rizkyby_${windowId}_selected_gpu`, id);
            saveConfig({ selected_gpu: id });
            if (typeof fetchStats === 'function') fetchStats();
        }

        let lastDisksList = [];

        function handleDiskScroll(e) {
            e.preventDefault();
            e.stopPropagation();
            if (!lastDisksList || lastDisksList.length <= 1) return;
            
            let currIdx = lastDisksList.findIndex(d => String(d.id) === String(selectedDiskId));
            if (currIdx === -1) currIdx = 0;
            if (e.deltaY > 0) {
                currIdx = (currIdx + 1) % lastDisksList.length;
            } else {
                currIdx = (currIdx - 1 + lastDisksList.length) % lastDisksList.length;
            }
            selectCustomDisk(lastDisksList[currIdx].id, e);
        }

        function updateDiskSelector(disksList, diskData) {
            lastDiskData = diskData;
            lastDisksList = disksList || [];
            const labelTextEl = document.getElementById('disk-label-text');
            let menu = document.getElementById('custom-disk-options');
            if (!menu) {
                menu = document.createElement('div');
                menu.id = 'custom-disk-options';
                menu.className = 'custom-options-menu';
                menu.style.webkitAppRegion = 'no-drag';
                document.getElementById('app-window').appendChild(menu);
            }

            if (!disksList || disksList.length === 0) {
                if (labelTextEl) labelTextEl.textContent = 'No storage detected';
                if (menu) menu.innerHTML = '<div class="custom-option-item">No disks detected</div>';
                return;
            }

            const savedDisk = safeStorage.getItem(`rizkyby_${windowId}_selected_disk`) || safeStorage.getItem('rizkyby_selected_disk');
            if (!selectedDiskId || !disksList.some(d => String(d.id) === String(selectedDiskId))) {
                const rootDisk = disksList.find(d => d.is_root === true || d.is_root === "true");
                if (rootDisk) {
                    selectedDiskId = rootDisk.id; // Kunci mutlak ke Disk Booter
                } else {
                    selectedDiskId = disksList[0].id;
                }
            }

            const curDisk = disksList.find(d => String(d.id) === String(selectedDiskId)) || disksList[0];
            if (labelTextEl && curDisk) {
                labelTextEl.textContent = `${curDisk.icon} ${curDisk.model} (${curDisk.size})${curDisk.is_root ? ' [OS Root]' : ''}`;
            }

            if (menu) {
                menu.innerHTML = disksList.map(d => `
                    <div class="custom-option-item ${String(d.id) === String(selectedDiskId) ? 'selected' : ''}" onclick="selectCustomDisk('${d.id}', event)">
                        ${d.icon} ${d.model} (${d.size})${d.is_root ? ' [OS Root]' : ''}
                    </div>
                `).join('');
            }

            renderSelectedDiskInfo(diskData);
        }

        function toggleDiskMenu(e) {
            if (e && typeof e.stopPropagation === 'function') e.stopPropagation();
            let menu = document.getElementById('custom-disk-options');
            const box = document.getElementById('custom-disk-select');
            if (!menu || !box) return;
            const isShowing = menu.classList.contains('show');
            document.querySelectorAll('.custom-options-menu').forEach(m => m.classList.remove('show'));
            if (!isShowing) {
                menu.classList.add('show'); 
                
                const rect = box.getBoundingClientRect();
                const isFs = document.querySelector('.card.fullscreen') !== null;
                const offsetTop = isFs ? 8 : 4;

                menu.style.position = 'fixed';
                menu.style.top = (rect.bottom + offsetTop) + 'px';
                menu.style.left = rect.left + 'px';
                menu.style.right = 'auto';
                menu.style.maxWidth = '90vw';
                
                const menuRect = menu.getBoundingClientRect();
                if (menuRect.right > window.innerWidth) {
                    menu.style.left = Math.max(10, window.innerWidth - menuRect.width - 10) + 'px';
                }
                
                menu.style.zIndex = '9990';
                menu.style.pointerEvents = 'auto';
            }
        }

        function selectCustomDisk(id, e) {
            if (e && typeof e.stopPropagation === 'function') e.stopPropagation();
            const menu = document.getElementById('custom-disk-options');
            if (menu) menu.classList.remove('show');
            selectedDiskId = id;
            safeStorage.setItem(`rizkyby_${windowId}_selected_disk`, id);
            saveConfig({ selected_disk: id });

            _prevTopDisk = ''; // Reset cache teks daftar proses agar langsung terbarui
            if (lastDiskData) renderSelectedDiskInfo(lastDiskData);
        }

        // 14 Dark Palettes (Warna Aksen Kaya & Tidak Monoton Kuning)
        const DARK_PALETTES = {
            cyberpunk: { name: "Neon Cyberpunk", icon: "🎨", blue: "#06b6d4", purple: "#ec4899", green: "#10b981", orange: "#eab308", cyan: "#38bdf8", red: "#f43f5e", bgGrad1: "#0a1128", bgGrad2: "#2d0b4e" },
            matrix:    { name: "Terminal Matrix", icon: "📟", blue: "#22c55e", purple: "#a855f7", green: "#10b981", orange: "#eab308", cyan: "#06b6d4", red: "#ef4444", bgGrad1: "#022c22", bgGrad2: "#081c15" },
            volcanic:  { name: "Volcanic Crimson", icon: "🌋", blue: "#38bdf8", purple: "#c084fc", green: "#10b981", orange: "#f97316", cyan: "#fb923c", red: "#ef4444", bgGrad1: "#450a0a", bgGrad2: "#180509" },
            gold:      { name: "Golden Luxury",   icon: "👑", blue: "#eab308", purple: "#a855f7", green: "#10b981", orange: "#f97316", cyan: "#06b6d4", red: "#f43f5e", bgGrad1: "#3b2203", bgGrad2: "#0f172a" },
            synthwave: { name: "Synthwave 80s",   icon: "🌆", blue: "#38bdf8", purple: "#d946ef", green: "#10b981", orange: "#f43f5e", cyan: "#a855f7", red: "#fb7185", bgGrad1: "#4c0519", bgGrad2: "#1e1b4b" },
            emerald:   { name: "Emerald Aurum",   icon: "🌲", blue: "#10b981", purple: "#8b5cf6", green: "#22c55e", orange: "#f59e0b", cyan: "#06b6d4", red: "#ef4444", bgGrad1: "#064e3b", bgGrad2: "#0f172a" },
            sunset:    { name: "Sunset Horizon",  icon: "🌅", blue: "#f59e0b", purple: "#ec4899", green: "#10b981", orange: "#f97316", cyan: "#38bdf8", red: "#ef4444", bgGrad1: "#7c2d12", bgGrad2: "#4a044e" },
            amethyst:  { name: "Amethyst Prism",  icon: "🔮", blue: "#8b5cf6", purple: "#c084fc", green: "#10b981", orange: "#f59e0b", cyan: "#06b6d4", red: "#f43f5e", bgGrad1: "#3b0764", bgGrad2: "#030712" },
            ocean:     { name: "Abyssal Deep",    icon: "🌊", blue: "#0284c7", purple: "#818cf8", green: "#10b981", orange: "#eab308", cyan: "#06b6d4", red: "#ef4444", bgGrad1: "#0c4a6e", bgGrad2: "#020617" },
            sakura:    { name: "Sakura Blossom",  icon: "🌸", blue: "#f43f5e", purple: "#fb7185", green: "#10b981", orange: "#f59e0b", cyan: "#38bdf8", red: "#e11d48", bgGrad1: "#881337", bgGrad2: "#1e1b4b" },
            glacier:   { name: "Glacier Frost",   icon: "🧊", blue: "#38bdf8", purple: "#a78bfa", green: "#34d399", orange: "#fbbf24", cyan: "#22d3ee", red: "#f87171", bgGrad1: "#164e63", bgGrad2: "#020617" },
            dracula:   { name: "Dracula Purple",  icon: "🧛", blue: "#bd93f9", purple: "#ff79c6", green: "#50fa7b", orange: "#ffb86c", cyan: "#8be9fd", red: "#ff5555", bgGrad1: "#282a36", bgGrad2: "#111217" },
            fox:       { name: "Cyber Fox",       icon: "🦊", blue: "#ff6b00", purple: "#a855f7", green: "#10b981", orange: "#f59e0b", cyan: "#38bdf8", red: "#ef4444", bgGrad1: "#7c2d12", bgGrad2: "#18181b" },
            monochrome:{ name: "Pure Silver",     icon: "🤍", blue: "#f8fafc", purple: "#94a3b8", green: "#10b981", orange: "#eab308", cyan: "#38bdf8", red: "#ef4444", bgGrad1: "#1e293b", bgGrad2: "#020617" }
        };

        // 14 Light Palettes (Warna Terang & Variatif)
        const LIGHT_PALETTES = {
            breeze:    { name: "Pastel Breeze",    icon: "🩵", blue: "#0284c7", purple: "#7c3aed", green: "#059669", orange: "#d97706", cyan: "#0891b2", red: "#dc2626", bgGrad1: "#bae6fd", bgGrad2: "#e0e7ff" },
            matcha:    { name: "Matcha Garden",   icon: "🍵", blue: "#16a34a", purple: "#7c3aed", green: "#15803d", orange: "#d97706", cyan: "#0d9488", red: "#dc2626", bgGrad1: "#bbf7d0", bgGrad2: "#fef08a" },
            volcanic:  { name: "Crimson Ruby",    icon: "🔥", blue: "#e11d48", purple: "#7c3aed", green: "#059669", orange: "#ea580c", cyan: "#0284c7", red: "#b91c1c", bgGrad1: "#fecdd3", bgGrad2: "#fed7aa" },
            gold:      { name: "Royal Amber",     icon: "👑", blue: "#d97706", purple: "#7c3aed", green: "#15803d", orange: "#ea580c", cyan: "#0284c7", red: "#dc2626", bgGrad1: "#fef08a", bgGrad2: "#e0e7ff" },
            nordic:    { name: "Nordic Frost",     icon: "❄️", blue: "#2563eb", purple: "#9333ea", green: "#0d9488", orange: "#d97706", cyan: "#0284c7", red: "#dc2626", bgGrad1: "#bfdbfe", bgGrad2: "#fbcfe8" },
            strawberry:{ name: "Strawberry Cream",icon: "🍓", blue: "#e11d48", purple: "#7c3aed", green: "#059669", orange: "#ea580c", cyan: "#0891b2", red: "#be123c", bgGrad1: "#fce7f3", bgGrad2: "#fed7aa" },
            citrus:    { name: "Citrus Sunshine", icon: "🍊", blue: "#ea580c", purple: "#7c3aed", green: "#16a34a", orange: "#d97706", cyan: "#0284c7", red: "#dc2626", bgGrad1: "#ffedd5", bgGrad2: "#bbf7d0" },
            lavender:  { name: "Lavender Glow",   icon: "💜", blue: "#7c3aed", purple: "#9333ea", green: "#059669", orange: "#d97706", cyan: "#0284c7", red: "#e11d48", bgGrad1: "#ede9fe", bgGrad2: "#cffafe" },
            cotton:    { name: "Cotton Candy",    icon: "🍬", blue: "#c026d3", purple: "#2563eb", green: "#059669", orange: "#ea580c", cyan: "#0891b2", red: "#e11d48", bgGrad1: "#fae8ff", bgGrad2: "#cffafe" },
            sage:      { name: "Nordic Sage",     icon: "🌿", blue: "#0f766e", purple: "#7c3aed", green: "#16a34a", orange: "#d97706", cyan: "#0d9488", red: "#dc2626", bgGrad1: "#ccfbf1", bgGrad2: "#fef08a" },
            espresso:  { name: "Espresso Cream",  icon: "☕", blue: "#b45309", purple: "#7c3aed", green: "#15803d", orange: "#ea580c", cyan: "#0284c7", red: "#b91c1c", bgGrad1: "#f5ebe0", bgGrad2: "#e0e7ff" },
            arctic:    { name: "Arctic Crystal",  icon: "💎", blue: "#0891b2", purple: "#7c3aed", green: "#059669", orange: "#ea580c", cyan: "#0284c7", red: "#e11d48", bgGrad1: "#cffafe", bgGrad2: "#fce7f3" },
            peach:     { name: "Peach Blossom",   icon: "🍑", blue: "#f97316", purple: "#7c3aed", green: "#16a34a", orange: "#ea580c", cyan: "#0284c7", red: "#e11d48", bgGrad1: "#ffedd5", bgGrad2: "#ede9fe" },
            paper:     { name: "Minimal Ink",     icon: "📄", blue: "#0f172a", purple: "#475569", green: "#15803d", orange: "#c2410c", cyan: "#0369a1", red: "#b91c1c", bgGrad1: "#e2e8f0", bgGrad2: "#cbd5e1" }
        };
        let currentPalette = 'cyberpunk';
        let currentThemeMode = 'dark';

        function getActivePalettes() {
            return currentThemeMode === 'light' ? LIGHT_PALETTES : DARK_PALETTES;
        }

        function initColorSelector() {
            const menu = document.getElementById('custom-color-options');
            if (!menu) return;
            const palettes = getActivePalettes();
            menu.innerHTML = Object.keys(palettes).map(key => {
                const pal = palettes[key];
                return `
                    <div class="custom-color-option-item ${key === currentPalette ? 'selected' : ''}" onclick="selectCustomColor('${key}', event)">
                        <span>${pal.icon}</span> ${pal.name}
                    </div>
                `;
            }).join('');
        }

        function toggleColorMenu(e) {
            if (e && typeof e.stopPropagation === 'function') {
                e.stopPropagation();
            }
            const menu = document.getElementById('custom-color-options');
            const box = document.getElementById('custom-color-select');
            if (!menu || !box) return;

            const isShowing = menu.classList.contains('show');
            if (isShowing) {
                menu.classList.remove('show');
            } else {
                // Tutup semua menu lain (disk/gpu) yang mungkin masih terbuka
                document.querySelectorAll('.custom-options-menu').forEach(m => m.classList.remove('show'));

                initColorSelector();

                // Tampilkan menu DULU supaya browser bisa menghitung lebarnya secara akurat
                menu.classList.add('show');

                const rect = box.getBoundingClientRect();
                menu.style.position = 'fixed';
                menu.style.top = (rect.bottom + 6) + 'px';

                // Set posisi natural: sejajar di kiri tombol
                menu.style.left = rect.left + 'px';
                menu.style.right = 'auto';
                menu.style.maxWidth = '90vw';

                // Pengecekan cerdas: Apakah ujung kanan menu nabrak batas kanan layar?
                const menuRect = menu.getBoundingClientRect();
                if (menuRect.right > window.innerWidth) {
                    // Kalau nabrak, dorong menunya ke kiri secukupnya biar aman masuk layar
                    menu.style.left = Math.max(10, window.innerWidth - menuRect.width - 10) + 'px';
                }

                menu.style.zIndex = '999999';
            }
        } // Hapus satu kurung kurawal nyasar di sini!

        function selectCustomColor(key, e) {
            if (e && typeof e.stopPropagation === 'function') {
                e.stopPropagation();
            }
            const menu = document.getElementById('custom-color-options');
            if (menu) menu.classList.remove('show');
            setPalette(key);
        }

        function setPalette(key) {
            const palettes = getActivePalettes();
            let targetKey = key;
            if (!palettes[targetKey]) {
                targetKey = Object.keys(palettes)[0];
            }
            currentPalette = targetKey;
            if (currentThemeMode === 'light') {
                safeStorage.setItem(`rizkyby_${windowId}_palette_light`, targetKey);
                saveConfig({ mode: currentThemeMode, palette_light: targetKey });
            } else {
                safeStorage.setItem(`rizkyby_${windowId}_palette_dark`, targetKey);
                saveConfig({ mode: currentThemeMode, palette_dark: targetKey });
            }
            const pal = palettes[targetKey];
            const root = document.documentElement;

            // Set 6 Aksen Warna Berbeda untuk Font, Label, Border, & Grafik
            root.style.setProperty('--accent-blue', pal.blue);
            root.style.setProperty('--accent-purple', pal.purple);
            root.style.setProperty('--accent-green', pal.green);
            root.style.setProperty('--accent-orange', pal.orange);
            root.style.setProperty('--accent-cyan', pal.cyan);
            root.style.setProperty('--accent-red', pal.red);

            // 2-Stop Linear Gradient Miring 135deg (Tanpa Duplikasi Variabel)
            const g1 = pal.bgGrad1 || (currentThemeMode === 'light' ? '#bae6fd' : '#0f172a');
            const g2 = pal.bgGrad2 || (currentThemeMode === 'light' ? '#e0e7ff' : '#2e1065');
            const appWin = document.getElementById('app-window') || document.body;
            appWin.style.backgroundImage = `linear-gradient(135deg, ${g1} 0%, ${g2} 100%)`;

            const glowColor = pal.blue || pal.purple || '#3b82f6';
            const shadowGlow = currentThemeMode === 'light'
                ? `0 8px 30px rgba(0, 0, 0, 0.08), 0 0 15px ${glowColor}20`
                : `0 10px 35px rgba(0, 0, 0, 0.5), 0 0 25px ${glowColor}2e`;

            const shadowHoverGlow = currentThemeMode === 'light'
                ? `0 12px 40px rgba(0, 0, 0, 0.15), 0 0 25px ${glowColor}35`
                : `0 15px 45px rgba(0, 0, 0, 0.7), 0 0 35px ${glowColor}48`;

            root.style.setProperty('--card-shadow', shadowGlow);
            root.style.setProperty('--card-shadow-hover', shadowHoverGlow);

            const labelText = document.getElementById('color-label-text');
            if (labelText) labelText.textContent = `${pal.icon} ${pal.name}`;

            initColorSelector();

            // Dynamic Chart dataset colors update
            try {
                if (cpuChart && cpuChart.data && cpuChart.data.datasets[0]) {
                    cpuChart.data.datasets[0].borderColor = pal.blue;
                    cpuChart.data.datasets[0].backgroundColor = pal.blue + '22';
                    cpuChart.update();
                }
            } catch(e) {}
            try {
                if (gpuChart && gpuChart.data && gpuChart.data.datasets.length >= 4) {
                    gpuChart.data.datasets[0].borderColor = pal.purple;
                    gpuChart.data.datasets[1].borderColor = pal.blue;
                    gpuChart.data.datasets[2].borderColor = pal.cyan;
                    gpuChart.data.datasets[3].borderColor = pal.orange;
                    gpuChart.update();
                }
            } catch(e) {}
            try {
                if (memChart && memChart.data && memChart.data.datasets.length >= 3) {
                    memChart.data.datasets[0].borderColor = pal.green;
                    memChart.data.datasets[1].borderColor = pal.orange;
                    memChart.data.datasets[2].borderColor = pal.red;
                    memChart.update();
                }
            } catch(e) {}
            try {
                if (netChart && netChart.data && netChart.data.datasets.length >= 2) {
                    netChart.data.datasets[0].borderColor = pal.cyan;
                    netChart.data.datasets[1].borderColor = pal.blue;
                    netChart.update();
                }
            } catch(e) {}
            try {
                if (diskChart && diskChart.data && diskChart.data.datasets.length >= 2) {
                    diskChart.data.datasets[0].borderColor = pal.orange;
                    diskChart.data.datasets[1].borderColor = pal.red;
                    diskChart.update();
                }
            } catch(e) {}
        }

        function setThemeMode(mode, targetPaletteKey) {
            currentThemeMode = mode;
            safeStorage.setItem(`rizkyby_${windowId}_mode`, mode);

            // Kunci atribut data-mode pada <html> dan <body>
            document.documentElement.setAttribute('data-mode', mode);
            if (document.body) document.body.setAttribute('data-mode', mode);

            let palKey = targetPaletteKey;
            if (!palKey) {
                if (mode === 'light') {
                    palKey = safeStorage.getItem(`rizkyby_${windowId}_palette_light`) || 'breeze';
                } else {
                    palKey = safeStorage.getItem(`rizkyby_${windowId}_palette_dark`) || 'cyberpunk';
                }
            }

            const darkPal = mode === 'dark' ? palKey : (safeStorage.getItem(`rizkyby_${windowId}_palette_dark`) || 'cyberpunk');
            const lightPal = mode === 'light' ? palKey : (safeStorage.getItem(`rizkyby_${windowId}_palette_light`) || 'breeze');

            saveConfig({
                mode: mode,
                palette_dark: darkPal,
                palette_light: lightPal
            });

            const btnDark = document.getElementById('btn-mode-dark');
            const btnLight = document.getElementById('btn-mode-light');

            const root = document.documentElement;
            if (mode === 'light') {
                if (btnDark) btnDark.classList.remove('active');
                if (btnLight) btnLight.classList.add('active');

                root.style.setProperty('--card-bg', 'rgba(255, 255, 255, 0.50)');
                root.style.setProperty('--card-hover-bg', 'rgba(255, 255, 255, 0.50)');
                root.style.setProperty('--glass-bg', 'rgba(255, 255, 255, 0.50)');
                root.style.setProperty('--glass-border', 'rgba(0, 0, 0, 0.12)');
                root.style.setProperty('--header-line', 'rgba(0, 0, 0, 0.08)');
                root.style.setProperty('--core-bg', 'rgba(0, 0, 0, 0.04)');
                root.style.setProperty('--list-bg', 'rgba(0, 0, 0, 0.03)');
                root.style.setProperty('--progress-bg', 'rgba(0, 0, 0, 0.08)');
                root.style.setProperty('--text-main', '#0f172a');
                root.style.setProperty('--text-muted', '#475569');
            } else {
                if (btnLight) btnLight.classList.remove('active');
                if (btnDark) btnDark.classList.add('active');

                root.style.setProperty('--card-bg', 'rgba(0, 0, 0, 0.50)');
                root.style.setProperty('--card-hover-bg', 'rgba(0, 0, 0, 0.50)');
                root.style.setProperty('--glass-bg', 'rgba(0, 0, 0, 0.50)');
                root.style.setProperty('--glass-border', 'rgba(255, 255, 255, 0.12)');
                root.style.setProperty('--header-line', 'rgba(255, 255, 255, 0.08)');
                root.style.setProperty('--core-bg', 'rgba(255, 255, 255, 0.04)');
                root.style.setProperty('--list-bg', 'rgba(255, 255, 255, 0.03)');
                root.style.setProperty('--progress-bg', 'rgba(255, 255, 255, 0.05)');
                root.style.setProperty('--text-main', '#f8fafc');
                root.style.setProperty('--text-muted', '#94a3b8');
            }

            const gridColor = mode === 'light' ? 'rgba(0, 0, 0, 0.08)' : 'rgba(255, 255, 255, 0.05)';
            const tickColor = mode === 'light' ? '#475569' : '#94a3b8';
            [cpuChart, gpuChart, memChart, netChart, diskChart].forEach(chart => {
                if (chart && chart.options && chart.options.scales && chart.options.scales.y) {
                    chart.options.scales.y.grid.color = gridColor;
                    chart.options.scales.y.ticks.color = tickColor;
                    chart.update();
                }
            });

            setPalette(palKey);

            // KUNCI REALTIME: Panggil pembaruan posisi & warna layer saat toggle GitHub aktif
            if (isEasterEggActive && typeof updateEasterEggPositions === 'function') {
                updateEasterEggPositions();
            }
        }

        document.addEventListener('click', function(e) {
            const menuDisk = document.getElementById('custom-disk-options');
            if (menuDisk && menuDisk.classList.contains('show')) {
                if (!e.target.closest('#custom-disk-select')) {
                    menuDisk.classList.remove('show');
                }
            }
            const menuGpu = document.getElementById('custom-gpu-options');
            if (menuGpu && menuGpu.classList.contains('show')) {
                if (!e.target.closest('#custom-gpu-select')) {
                    menuGpu.classList.remove('show');
                }
            }
            const menuColor = document.getElementById('custom-color-options');
            if (menuColor && menuColor.classList.contains('show')) {
                if (!e.target.closest('#custom-color-select')) {
                    menuColor.classList.remove('show');
                }
            }
        });

        function renderSelectedDiskInfo(diskData) {
            const disksList = diskData.disks || [];
            if (!disksList || disksList.length === 0) {
                const nameEl = document.getElementById('selected-disk-name');
                if (nameEl) nameEl.textContent = 'No storage device detected';
                const s1 = document.getElementById('val-stat-1'); if (s1) s1.textContent = 'N/A';
                const s2 = document.getElementById('val-stat-2'); if (s2) s2.textContent = 'N/A';
                const s3 = document.getElementById('val-stat-3'); if (s3) s3.textContent = 'N/A';
                const s4 = document.getElementById('val-stat-4'); if (s4) s4.textContent = 'N/A';
                return;
            }

            // Cari disk yang cocok secara presisi (id atau dev)
            const disk = disksList.find(d => String(d.id) === String(selectedDiskId) || String(d.dev) === String(selectedDiskId)) || disksList[0];
            if (!disk) return;

            // Update custom auto-scrolling selector label text
            const labelTextEl = document.getElementById('disk-label-text');
            if (labelTextEl) {
                const fullText = `${disk.icon} ${disk.model} (${disk.size})${disk.is_root ? ' [OS Root]' : ''}`;
                if (labelTextEl.textContent !== fullText) {
                    labelTextEl.textContent = fullText;
                    const scrollContainer = document.getElementById('custom-disk-label');
                    if (scrollContainer) {
                        scrollContainer.dataset.scrolling = "";
                        scrollContainer.scrollLeft = 0;
                        setTimeout(() => applyAutoScroll(), 50);
                    }
                }
            }

            // Update menu items selection state
            const menu = document.getElementById('custom-disk-options');
            if (menu) {
                Array.from(menu.children).forEach(child => {
                    if (child.getAttribute('onclick') && child.getAttribute('onclick').includes(`'${disk.id}'`)) {
                        child.classList.add('selected');
                    } else {
                        child.classList.remove('selected');
                    }
                });
            }

            // --- GANTI BLOK PUSH DATA & CHART UPDATE DI renderSelectedDiskInfo() DENGAN INI ---
            const curReadRate = Math.max(0, Number(disk.read_rate) || 0);
            const curWriteRate = Math.max(0, Number(disk.write_rate) || 0);

            const rEl = document.getElementById('disk-read');
            const wEl = document.getElementById('disk-write');
            if (rEl) rEl.textContent = formatSpeed(curReadRate);
            if (wEl) wEl.textContent = formatSpeed(curWriteRate);

            // Update data array riwayat
            // Update data array riwayat per disk independen
            const curDiskId = disk.id || 'default';
            if (!diskHistoryMap[curDiskId]) {
                diskHistoryMap[curDiskId] = {
                    read: Array(MAX_DATA_POINTS).fill(0),
                    write: Array(MAX_DATA_POINTS).fill(0)
                };
            }
            const history = diskHistoryMap[curDiskId];
            history.read.push(curReadRate);
            history.read.shift();
            history.write.push(curWriteRate);
            history.write.shift();

            diskReadData = history.read;
            diskWriteData = history.write;

            if (diskChart) {
                diskChart.data.datasets[0].data = diskReadData;
                diskChart.data.datasets[1].data = diskWriteData;
                const maxSpeed = Math.max(...diskReadData, ...diskWriteData);
                const suggestedTop = maxSpeed > 0 ? Math.ceil(maxSpeed * 1.25) : (100 * 1024);

                if (diskChart.options.scales && diskChart.options.scales.y) {
                    diskChart.options.scales.y.suggestedMax = suggestedTop;
                    diskChart.options.scales.y.min = 0;
                }
                diskChart.update('none');
            }

            // Right column dynamic telemetry
            const diskTitleEl = document.getElementById('selected-disk-name');
            if (diskTitleEl) {
                diskTitleEl.innerHTML = `${disk.icon} ${disk.name} ${disk.is_root ? '<span style="color:var(--accent-green); font-size:0.7em;">[OS Linux]</span>' : ''}`;
            }
            
            // Dynamic Row Labels tailored to Drive Category
            const transportStr = (disk.transport || (disk.is_ssd ? (disk.id.includes('nvme') ? 'NVMe' : 'USB/SATA') : 'USB')).toUpperCase();
            
            if (disk.is_ssd) {
                // 1. Kasta Solid State (NVMe / SATA SSD / Portable SSD)
                document.getElementById('label-stat-1').textContent = 'TBW Used (Total Writes)';
                document.getElementById('val-stat-1').textContent = disk.tbw_str;
                document.getElementById('label-stat-2').textContent = 'TBW Remaining Life';
                document.getElementById('val-stat-2').textContent = disk.remaining_str || 'N/A';
                
            } else if (disk.is_hdd) {
                // 2. Kasta Mekanikal (Hard Disk Rotasional / Platter)
                document.getElementById('label-stat-1').textContent = 'Platter & Spindle Speed';
                document.getElementById('val-stat-1').textContent = disk.tbw_str;
                document.getElementById('label-stat-2').textContent = 'Transport Interface';
                document.getElementById('val-stat-2').textContent = transportStr + ' Bus Interface';
                
            } else if (disk.is_flash) {
                // 3. Kasta Removable (Flashdisk / SD Card Micro)
                document.getElementById('label-stat-1').textContent = 'Flash Storage Protocol';
                document.getElementById('val-stat-1').textContent = disk.tbw_str;
                document.getElementById('label-stat-2').textContent = 'Removable Media Status';
                document.getElementById('val-stat-2').textContent = 'Hot-Pluggable USB Mass Storage';
                
            } else {
                // 4. Kasta Eksotis & LTO (Tape Drive, Optical CD/DVD, Virtual RAMDisk, dsb)
                document.getElementById('label-stat-1').textContent = 'Device Telemetry / Status';
                document.getElementById('val-stat-1').textContent = disk.tbw_str || 'Generic Block Storage';
                document.getElementById('label-stat-2').textContent = 'Hardware Architecture';
                document.getElementById('val-stat-2').textContent = 'Sequential / Specialty Media';
            }
            
            // Row 3: Drive Type & Transport
            document.getElementById('label-stat-3').textContent = 'Drive Type & Transport';
            document.getElementById('val-stat-3').textContent = `${disk.type} (${transportStr})`;
            
            // Row 4: SMART Health / Temp
            document.getElementById('label-stat-4').textContent = 'SMART Health / Temp';
            document.getElementById('val-stat-4').textContent = `${disk.health_str} | ${disk.temp_str}`;

            // Row 5: Drive Capacity & Node
            const stat5El = document.getElementById('val-stat-5');
            if (stat5El) stat5El.textContent = disk.capacity_str || `${disk.size} (/dev/${disk.dev})`;

            // Row 6: Partition Used & Free Space
            const stat6El = document.getElementById('val-stat-6');
            if (stat6El) stat6El.textContent = disk.usage_str || 'Raw Block Storage';
            
            // Build rich English hover tooltip for Disk Card
            let tooltipLines = `<strong>💽 Connected Storage Hierarchy (${disksList.length} Disks):</strong><br>`;
            disksList.forEach((d, idx) => {
                const isSelected = (d.id === disk.id);
                const tag = isSelected ? " <span style='color:var(--accent-orange); font-weight:bold;'>➔ ACTIVE</span>" : "";
                const osTag = d.is_root ? " [OS Root]" : "";
                tooltipLines += `${idx+1}. ${d.icon} <strong>${d.model}</strong> (${d.size})${osTag}${tag}<br>&nbsp;&nbsp;&nbsp;&nbsp;Type: ${d.type} | ${d.tbw_str}<br>`;
            });
            window.hardwareTooltips.ssd = tooltipLines;

            // Update RAW detail view instantly for the active disk with pure monospaced text
            const detailsEl = document.getElementById('disk-details');
            if (detailsEl && disk && disk.detail_text) {
                detailsEl.textContent = disk.detail_text;
            }
        }

        // Enable Mouse Wheel scrolling on disk select dropdown
        setTimeout(() => {
            const diskSelEl = document.getElementById('disk-select');
            if (diskSelEl) {
                diskSelEl.addEventListener('wheel', (e) => {
                    e.preventDefault();
                    if (e.deltaY > 0 && diskSelEl.selectedIndex < diskSelEl.options.length - 1) {
                        diskSelEl.selectedIndex++;
                        onDiskSelect(diskSelEl.value);
                    } else if (e.deltaY < 0 && diskSelEl.selectedIndex > 0) {
                        diskSelEl.selectedIndex--;
                        onDiskSelect(diskSelEl.value);
                    }
                }, { passive: false });
            }
        }, 500);

		// =====================================================================
        // PENGATURAN INTERVAL TELEMETRI & KONTROL AUTOSCROLL CYCLE (▶ ⏸ ■)
        // =====================================================================
        let g_autoscroll_mode = safeStorage.getItem(`rizkyby_${windowId}_autoscroll_mode`) || 'on';
        let g_telemetry_interval_sec = parseFloat(safeStorage.getItem(`rizkyby_${windowId}_telemetry_sec`)) || 1.0;
        if (isNaN(g_telemetry_interval_sec) || g_telemetry_interval_sec < 0.5) g_telemetry_interval_sec = 1.0;

        let g_telemetry_timer = null;

        function updateAutoscrollButtonUI() {
            const btn = document.getElementById('btn-autoscroll-toggle');
            if (!btn) return;
            btn.removeAttribute('title'); // Hapus tooltip kotak hitam bawaan browser
            if (g_autoscroll_mode === 'on') {
                btn.innerHTML = '▶';
                btn.style.color = 'var(--accent-blue)';
                btn.style.textShadow = '0 0 10px var(--accent-blue)';
            } else if (g_autoscroll_mode === 'smart') {
                btn.innerHTML = '⏸';
                btn.style.color = 'var(--accent-orange)';
                btn.style.textShadow = '0 0 10px var(--accent-orange)';
            } else {
                btn.innerHTML = '■';
                btn.style.color = 'var(--text-muted)';
                btn.style.textShadow = 'none';
            }
        }

        let g_smart_unfocused_active = false;

        function resumeSmartAutoscroll() {
            if (isEasterEggActive) {
                if (typeof resumeEasterCreditsScroll === 'function') resumeEasterCreditsScroll();
                if (typeof resumeChangelogHScroll === 'function') resumeChangelogHScroll();
                document.querySelectorAll('.changelog-card-body').forEach(b => {
                    if (typeof runCardVerticalScroll === 'function') runCardVerticalScroll(b);
                });
            } else if (document.body.classList.contains('show-about-panel')) {
                if (typeof resumeAboutCreditsScroll === 'function') resumeAboutCreditsScroll();
            }
        }

        function pauseSmartAutoscroll() {
            if (typeof stopAboutAutoScroll === 'function') stopAboutAutoScroll();
            if (typeof stopEasterCreditsScroll === 'function') stopEasterCreditsScroll();
            if (typeof stopChangelogHScroll === 'function') stopChangelogHScroll();
            document.querySelectorAll('.changelog-card-body').forEach(b => {
                if (typeof stopCardVerticalScroll === 'function') stopCardVerticalScroll(b);
            });
        }

        // Fungsi helper cek izin autoscroll global (Dashboard + Credit + Changelog)
        function isAutoscrollAllowed() {
            if (g_autoscroll_mode === 'on') return true;
            if (g_autoscroll_mode === 'smart') {
                // Kondisi 1: Window aktif / fokus
                // Kondisi 2: Disabled window yang sedang di-hover + ada interaksi mouse
                return document.hasFocus() || g_smart_unfocused_active;
            }
            return false;
        }

        function cycleAutoscrollMode() {
            if (g_autoscroll_mode === 'on') {
                g_autoscroll_mode = 'smart';
                showCenterToast('⏸ Autoscroll: SMART (Active or Hover Interaction)');
                if (isAutoscrollAllowed()) {
                    resumeSmartAutoscroll();
                } else {
                    pauseSmartAutoscroll();
                }
            } else if (g_autoscroll_mode === 'smart') {
                g_autoscroll_mode = 'off';
                g_smart_unfocused_active = false;
                showCenterToast('■ Autoscroll: OFF (Disabled)');
                pauseSmartAutoscroll();
            } else {
                g_autoscroll_mode = 'on';
                showCenterToast('▶ Autoscroll: ALWAYS ON');
                resumeSmartAutoscroll();
            }
            safeStorage.setItem(`rizkyby_${windowId}_autoscroll_mode`, g_autoscroll_mode);
            updateAutoscrollButtonUI();
        }

        // Listener saat window fokus / blur
        window.addEventListener('focus', () => {
            if (g_autoscroll_mode === 'smart') {
                resumeSmartAutoscroll();
            }
        });
        window.addEventListener('blur', () => {
            g_smart_unfocused_active = false;
            if (g_autoscroll_mode === 'smart') {
                pauseSmartAutoscroll();
            }
        });

        // Tangkap event interaktif di disabled window: scroll wheel & klik (kiri, tengah, kanan)
        function triggerSmartMouseActivity() {
            if (g_autoscroll_mode === 'smart' && !document.hasFocus()) {
                if (!g_smart_unfocused_active) {
                    g_smart_unfocused_active = true;
                    resumeSmartAutoscroll();
                }
            }
        }
        window.addEventListener('wheel', triggerSmartMouseActivity, { passive: true });
        window.addEventListener('mousedown', triggerSmartMouseActivity);
        window.addEventListener('auxclick', triggerSmartMouseActivity);
        window.addEventListener('contextmenu', triggerSmartMouseActivity);

        // Saat cursor meninggalkan window (hover pergi), matikan autoscroll
        function onSmartMouseLeaveWindow(e) {
            if (!e.relatedTarget && !e.toElement) {
                if (g_smart_unfocused_active) {
                    g_smart_unfocused_active = false;
                    if (g_autoscroll_mode === 'smart' && !document.hasFocus()) {
                        pauseSmartAutoscroll();
                    }
                }
            }
        }
        document.addEventListener('mouseleave', onSmartMouseLeaveWindow);
        window.addEventListener('mouseout', onSmartMouseLeaveWindow);

        function setTelemetryIntervalSeconds(sec, showToast = true) {
            if (sec < 0.5) sec = 0.5; // Minimal 0.5s (500ms)
            sec = Math.round(sec * 10) / 10;
            g_telemetry_interval_sec = sec;
            safeStorage.setItem(`rizkyby_${windowId}_telemetry_sec`, sec.toString());

            const inputEl = document.getElementById('input-telemetry-interval');
            const displayEl = document.getElementById('interval-display-ms');
            if (inputEl) inputEl.value = sec.toFixed(1) + 's';
            if (displayEl) displayEl.textContent = Math.round(sec * 1000) + 'ms';

            // Restart interval telemetri
            if (g_telemetry_timer) clearInterval(g_telemetry_timer);
            g_telemetry_timer = setInterval(fetchStats, Math.round(sec * 1000));

            // Kirim interval baru ke backend server C++
            fetch('/api/config', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ window_id: windowId, telemetry_interval: Math.round(sec * 1000) })
            }).catch(() => {});

            if (showToast) {
                showCenterToast(`⏱️ Telemetry Rate: ${sec.toFixed(1)}s (${Math.round(sec * 1000)}ms) Applied`);
            }
        }

        function stepInterval(direction) {
            let cur = g_telemetry_interval_sec;
            let step = 0.5;
            if (direction > 0) {
                if (cur < 0.99) step = 0.1;
                else if (cur < 1.99) step = 0.2;
                else step = 0.5;
            } else {
                if (cur <= 1.01) step = 0.1;
                else if (cur <= 2.01) step = 0.2;
                else step = 0.5;
            }
            let next = cur + (direction * step);
            if (next < 0.5) next = 0.5;
            next = Math.round(next * 10) / 10;
            g_telemetry_interval_sec = next;

            const inputEl = document.getElementById('input-telemetry-interval');
            const displayEl = document.getElementById('interval-display-ms');
            if (inputEl) inputEl.value = next.toFixed(1) + 's';
            if (displayEl) displayEl.textContent = Math.round(next * 1000) + 'ms';
        }

        function applyTelemetryInterval() {
            const inputEl = document.getElementById('input-telemetry-interval');
            if (inputEl) {
                let clean = inputEl.value.toLowerCase().replace('ms', '').replace('s', '').trim();
                let num = parseFloat(clean);
                if (isNaN(num)) num = 1.0;
                if (num >= 50) num = num / 1000;
                setTelemetryIntervalSeconds(num, true);
            }
            closeTelemetryIntervalPopup();
        }

        function closeTelemetryIntervalPopup() {
            const popup = document.getElementById('telemetry-interval-popup');
            if (popup && popup.classList.contains('visible')) {
                popup.classList.remove('visible');
                setTimeout(() => {
                    if (!popup.classList.contains('visible')) popup.style.display = 'none';
                }, 220);
            }
        }

        function openTelemetryIntervalPopup(e) {
            if (e) { e.preventDefault(); e.stopPropagation(); }
            const popup = document.getElementById('telemetry-interval-popup');
            const btn = document.getElementById('btn-autoscroll-toggle');
            if (!popup || !btn) return;

            if (popup.classList.contains('visible')) {
                closeTelemetryIntervalPopup();
                return;
            }

            popup.style.display = 'flex';

            const rect = btn.getBoundingClientRect();
            const popupW = popup.offsetWidth || 230;
            const popupH = popup.offsetHeight || 80;

            let left = rect.left + (rect.width / 2) - (popupW / 2);
            let top = rect.bottom + 8;

            // Anti-mentok tepi window
            if (left + popupW > window.innerWidth - 12) left = window.innerWidth - popupW - 12;
            if (left < 12) left = 12;
            if (top + popupH > window.innerHeight - 12) top = rect.top - popupH - 8;
            if (top < 12) top = 12;

            popup.style.left = Math.round(left) + 'px';
            popup.style.top = Math.round(top) + 'px';

            // Picu Animasi Fade In
            requestAnimationFrame(() => {
                popup.classList.add('visible');
            });

            const input = document.getElementById('input-telemetry-interval');
            const displayEl = document.getElementById('interval-display-ms');
            if (input) {
                input.value = g_telemetry_interval_sec.toFixed(1) + 's';
                setTimeout(() => { input.focus(); input.select(); }, 50);

                // Tambahkan Listener Roda Mouse (Scroll Up: Tambah | Scroll Down: Kurang)
                if (!input._hasWheelHandler) {
                    input._hasWheelHandler = true;
                    input.addEventListener('wheel', (we) => {
                        we.preventDefault();
                        we.stopPropagation();
                        if (we.deltaY < 0) stepInterval(1);
                        else if (we.deltaY > 0) stepInterval(-1);
                    }, { passive: false });
                }
            }
            if (displayEl) displayEl.textContent = Math.round(g_telemetry_interval_sec * 1000) + 'ms';
        }

        // Klik di luar popup -> otomatis fade out tutup
        document.addEventListener('click', (e) => {
            const popup = document.getElementById('telemetry-interval-popup');
            if (popup && popup.classList.contains('visible')) {
                if (!e.target.closest('#telemetry-interval-popup') && !e.target.closest('#btn-autoscroll-toggle')) {
                    closeTelemetryIntervalPopup();
                }
            }
        });

        // Jalankan polling telemetri perdana & sinkronkan tombol
        updateAutoscrollButtonUI();
        setTelemetryIntervalSeconds(g_telemetry_interval_sec, false);
        fetchStats();
    
        // --- AUTO-SCROLL LOGIC ---
        function applyAutoScroll() {
            const easeInOutCubic = t => t < 0.5 ? 4 * t * t * t : 1 - Math.pow(-2 * t + 2, 3) / 2;
            const inverseEaseInOutCubic = r => r <= 0.5 ? Math.cbrt(r / 4) : 1 - Math.cbrt((1 - r) * 2) / 2;
            
            function calcDuration(max, isVert) {
                if (isVert) {
                    return Math.max(5000, Math.min(60000, 4000 + max * 30));
                } else {
                    return Math.max(1500, Math.min(12000, 1000 + max * 40));
                }
            }
            
            function startScrollAnimation(el, isVertical) {
                if (el.dataset.scrolling === "true") return;
                let scrollMax = isVertical ? (el.scrollHeight - el.clientHeight) : (el.scrollWidth - el.clientWidth);
                if (scrollMax <= 0) return;
                
                let isRightAligned = el.classList.contains('sensors-badge') || el.classList.contains('sensors-badge-wrapper') || el.classList.contains('gpu-selector-wrapper') || el.classList.contains('disk-selector-wrapper');
                if (isRightAligned && !el._hasInitializedScroll) {
                    el.scrollLeft = scrollMax;
                    el._hasInitializedScroll = true;
                }
                el.dataset.scrolling = "true";
                let startTime = performance.now();
                let paused = false;
                let direction = 1;
                let idleTimer = null;
                let duration = calcDuration(scrollMax, isVertical);
                
                function resumeAutoScroll() {
                    paused = false;
                    el._manualScrollActive = false;
                    scrollMax = isVertical ? (el.scrollHeight - el.clientHeight) : (el.scrollWidth - el.clientWidth);
                    if (scrollMax > 0) {
                        duration = calcDuration(scrollMax, isVertical);
                        let currentPos = isVertical ? el.scrollTop : el.scrollLeft;
                        let ratio = Math.max(0, Math.min(1, currentPos / scrollMax));
                        let effectiveRatio = isRightAligned ? (1 - ratio) : ratio;
                        let linearRatio = inverseEaseInOutCubic(effectiveRatio);
                        let cycle = direction === 1 ? linearRatio : (2 - linearRatio);
                        startTime = performance.now() - (cycle * duration);
                    } else {
                        startTime = performance.now();
                    }
                }
                
                function triggerManualScroll(e) {
                    if (e && e.target && e.target.closest('#disk-select, .disk-select-dropdown, #custom-disk-select, #custom-disk-options, .custom-disk-options-menu, #custom-color-select, .custom-color-options-menu')) {
                        return;
                    }
                    paused = true;
                    el._manualScrollActive = true;
                    if (e && (e.deltaY || e.deltaX)) {
                        let dy = e.deltaY ? e.deltaY * 0.45 : 0;
                        let dx = e.deltaX ? e.deltaX * 0.45 : 0;
                        
                        if (isVertical) {
                            el.scrollTop += dy;
                        } else {
                            el.scrollLeft += (dx || dy);
                        }
                    }
                    if (idleTimer) clearTimeout(idleTimer);
                    idleTimer = setTimeout(resumeAutoScroll, 5000);
                }

                function onEnter() {
                    paused = true;
                }
                
                function onLeave() {
                    if (idleTimer) clearTimeout(idleTimer);
                    resumeAutoScroll();
                }
                
                el.addEventListener('wheel', triggerManualScroll, { passive: true });
                el.addEventListener('touchmove', triggerManualScroll, { passive: true });
                el.addEventListener('mouseenter', onEnter);
                el.addEventListener('mouseleave', onLeave);
                
                function animate(currentTime) {
                    if (!el.isConnected) {
                        el.dataset.scrolling = "";
                        if (idleTimer) clearTimeout(idleTimer);
                        el.removeEventListener('wheel', triggerManualScroll);
                        el.removeEventListener('touchmove', triggerManualScroll);
                        el.removeEventListener('mouseenter', onEnter);
                        el.removeEventListener('mouseleave', onLeave);
                        return;
                    }
                    
                    scrollMax = isVertical ? (el.scrollHeight - el.clientHeight) : (el.scrollWidth - el.clientWidth);
                    if (scrollMax <= 0) {
                        el.dataset.scrolling = "";
                        if (idleTimer) clearTimeout(idleTimer);
                        el.removeEventListener('wheel', triggerManualScroll);
                        el.removeEventListener('touchmove', triggerManualScroll);
                        el.removeEventListener('mouseenter', onEnter);
                        el.removeEventListener('mouseleave', onLeave);
                        return; 
                    }
                    
                    // Cek izin scroll sesuai mode (mematuhi kondisi fokus maupun hover interaktif):
                    let isScrollPermitted = isAutoscrollAllowed();

                    if (!paused && isScrollPermitted) {
                        duration = calcDuration(scrollMax, isVertical);
                        let elapsed = currentTime - startTime;
                        let cycle = (elapsed / duration) % 2;
                        direction = cycle <= 1 ? 1 : -1;
                        let linearRatio = cycle <= 1 ? cycle : (2 - cycle);
                        let easedRatio = easeInOutCubic(linearRatio);
                        let pos = isRightAligned 
                            ? (1 - easedRatio) * scrollMax 
                            : easedRatio * scrollMax;
                        
                        if (isVertical) {
                            el.scrollTop = pos;
                        } else {
                            el.scrollLeft = pos;
                        }
                    }
                    
                    requestAnimationFrame(animate);
                }
                requestAnimationFrame(animate);
            }

            document.querySelectorAll('.list-name, .metric-header:not(.no-autoscroll), .metric-value:not(.no-autoscroll), .auto-scroll-x, h1, h2, .sensor-item, .disk-selector-wrapper').forEach(el => {
                if (el.closest('#credit-frozen')) return;
                if (el.closest('#about-panel') && !document.body.classList.contains('show-about-panel')) return;
                startScrollAnimation(el, false);
            });

            document.querySelectorAll('.card-content, .list-container, .cores-grid, .cores-wrapper, #cpu-cores-wrapper, .card-inner-details').forEach(el => {
                if (el.closest('#credit-frozen')) return;
                if (el.closest('#about-panel') && !document.body.classList.contains('show-about-panel')) return;
                startScrollAnimation(el, true);
            });
        }
        
        setInterval(applyAutoScroll, 1000);

        // GLOBAL wheel interceptor for .card-inner-details
        document.addEventListener('wheel', function(e) {
            if (e.ctrlKey) return;
            
            let target = e.target;
            let detailsEl = null;
            while (target && target !== document) {
                if (target.classList && target.classList.contains('card-inner-details')) {
                    detailsEl = target;
                    break;
                }
                if (target.classList && target.classList.contains('card') && target.classList.contains('show-details')) {
                    detailsEl = target.querySelector('.card-inner-details');
                    break;
                }
                target = target.parentElement;
            }
            
            if (detailsEl && detailsEl.offsetParent !== null) {
                let dy = e.deltaY ? e.deltaY * 0.45 : 0;
                detailsEl.scrollTop += dy;
                
                detailsEl._manualScrollActive = true;
                if (detailsEl._manualIdleTimer) clearTimeout(detailsEl._manualIdleTimer);
                detailsEl._manualIdleTimer = setTimeout(() => {
                    detailsEl._manualScrollActive = false;
                }, 5000);
            }
        }, { passive: true });

        // Fungsi pembantu masking dimatikan total agar toast tidak terpotong gradien
        function applyPopupMask(element) {
            if (element) {
                element.style.webkitMaskImage = '';
                element.style.maskImage = '';
            }
        }

        function showCardToast(targetEl, msg) {
            let toast = document.getElementById('floating-toast');
            if (!toast) {
                toast = document.createElement('div');
                toast.id = 'floating-toast';
                toast.className = 'card-toast';
                document.getElementById('app-window').appendChild(toast);
            }

            const accentColor = getComputedStyle(document.documentElement).getPropertyValue('--accent-blue').trim();
            if (accentColor) {
                toast.style.backgroundColor = accentColor;
            }

            // Only treat this as "the GitHub button" when the actual copy target
            // IS a GitHub element (real or the easter-egg clone). Being in easter-egg
            // mode alone must NOT force every toast (tooltip/card/header) to the
            // GitHub button's position.
            let activeGitBtn = null;
            const clickedGithubEl = targetEl && (targetEl.closest('.github-link') || targetEl.closest('.easter-github-clone'));
            if (clickedGithubEl) {
                // Prefer the visible clone's rect if the easter egg overlay is active
                // (the original button may be hidden/faded out behind it).
                activeGitBtn = (isEasterEggActive && easterEggClone) ? easterEggClone : clickedGithubEl;
            }

            const isGithubEl = !!activeGitBtn;
            const isHeaderEl = !isGithubEl && targetEl && (targetEl.closest('header') || (window._lastHoveredTooltipEl && window._lastHoveredTooltipEl.closest('header')));
            const isTooltipVisible = !isGithubEl && globalTooltip && (globalTooltip.style.opacity === '1' || parseFloat(globalTooltip.style.opacity) > 0);

            let centerX, centerY;

            // 1. ACTIVE GITHUB BUTTON: ALWAYS CENTER DIRECTLY ON THE VISIBLE ACTIVE GITHUB BUTTON
            if (isGithubEl && activeGitBtn) {
                const rect = activeGitBtn.getBoundingClientRect();
                centerX = rect.left + (rect.width / 2);
                centerY = rect.top + (rect.height / 2);
            }
            // 2. HEADER BUTTONS WITH AN ACTIVE TOOLTIP: DIRECTLY CENTERED ON FLOATING TOOLTIP
            else if (isTooltipVisible) {
                const ttRect = globalTooltip.getBoundingClientRect();
                centerX = ttRect.left + (ttRect.width / 2);
                centerY = ttRect.top + (ttRect.height / 2);
            }
            // 3. OTHER ELEMENTS: CENTERED DIRECTLY ON TARGET
            else if (targetEl && targetEl.getBoundingClientRect) {
                const rect = targetEl.getBoundingClientRect();
                centerX = rect.left + (rect.width / 2);
                centerY = rect.top + (rect.height / 2);
            } else {
                centerX = window.innerWidth / 2;
                centerY = window.innerHeight / 2;
            }

            toast.style.left = centerX + 'px';
            toast.style.top = centerY + 'px';
            toast.style.transform = 'translate(-50%, -50%)';
            toast.style.fontSize = 'clamp(0.72rem, 0.9vw, 0.85rem)';
            toast.style.padding = '0.4rem 0.9rem';

            // Bersihkan total masking agar TIDAK PERNAH terpotong gradien
            toast.style.webkitMaskImage = '';
            toast.style.maskImage = '';

            // Prioritas Z-Index: Header & Tombol GitHub selalu di atas layer blur/Easter Egg
            if (isHeaderEl || isGithubEl) {
                toast.style.zIndex = '2000001';
            } else {
                toast.style.zIndex = isEasterEggActive ? '150000' : '2000001';
            }

            toast.textContent = msg;
            toast.classList.add('show');
            setTimeout(() => { toast.classList.remove('show'); }, 2000);
        }

        function showCenterToast(msg) {
            let toast = document.getElementById('center-screen-toast');
            if (!toast) {
                toast = document.createElement('div');
                toast.id = 'center-screen-toast';
                toast.className = 'card-toast';
                document.getElementById('app-window').appendChild(toast);
            }
            // Pastikan warna teks toast tengah mengikuti tema saat ini
            toast.style.color = currentThemeMode === 'light' ? '#ffffff' : '#000000';
            
            const accentColor = getComputedStyle(document.documentElement).getPropertyValue('--accent-blue').trim() || '#3b82f6';
            toast.style.backgroundColor = accentColor;
            toast.style.position = 'fixed';
            toast.style.left = '50%';
            toast.style.top = '50%';
            toast.style.transform = 'translate(-50%, -50%)';
            toast.style.zIndex = '999999';
            toast.style.fontSize = 'clamp(0.9rem, 1.2vw, 1.15rem)';
            toast.style.fontWeight = '700';
            toast.style.padding = '0.8rem 1.8rem';
            toast.style.borderRadius = '1.2rem';
            toast.style.boxShadow = '0 12px 35px rgba(0, 0, 0, 0.7), 0 0 30px ' + accentColor;
            toast.style.pointerEvents = 'none';
            toast.style.transition = 'opacity 0.25s ease, transform 0.25s ease';
            
            toast.textContent = msg;
            toast.classList.add('show');
            if (toast._hideTimer) clearTimeout(toast._hideTimer);
            toast._hideTimer = setTimeout(() => { toast.classList.remove('show'); }, 2200);
        }

        async function copyText(targetEl, text, customToastMsg) {
            const toastMsg = customToastMsg || "Copied Data to Clipboard!";

            // 1. Coba frontend Clipboard API dulu (lebih andal di Linux)
            try {
                if (navigator.clipboard && navigator.clipboard.writeText) {
                    await navigator.clipboard.writeText(text);
                    showCardToast(targetEl, toastMsg);
                    return;
                }
            } catch (e) {
                console.warn('Frontend clipboard failed, fallback to backend', e);
            }

            // 2. Fallback ke backend (untuk lingkungan tanpa izin clipboard)
            try {
                const res = await fetch('/api/copy', {
                    method: 'POST',
                    headers: { 'Content-Type': 'text/plain; charset=utf-8' },
                    body: text
                });
                if (res.ok) {
                    showCardToast(targetEl, toastMsg);
                    return;
                }
            } catch (e) {
                console.error('Backend copy failed', e);
            }

            // 3. Fallback paling tua (document.execCommand) – jarang dipakai
            try {
                const ta = document.createElement('textarea');
                ta.value = text;
                document.body.appendChild(ta);
                ta.select();
                document.execCommand('copy');
                showCardToast(targetEl, toastMsg);
            } catch (e) {
                showCardToast(targetEl, 'Gagal menyalin ke clipboard');
            } finally {
                const ta = document.querySelector('textarea');
                if (ta) ta.remove();
            }
        }

        function saveDetailsState() {
            const activeCards = [];
            document.querySelectorAll('.card.show-details').forEach(c => {
                const cls = Array.from(c.classList).find(l => l.startsWith('card-'));
                if (cls) activeCards.push(cls);
            });
            // 1. Simpan langsung ke Local Storage (string JSON array)
            safeStorage.setItem(`rizkyby_${windowId}_details`, JSON.stringify(activeCards));

            // 2. Kirim langsung ke backend
            fetch('/api/config', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ window_id: parseInt(windowId, 10), details: activeCards })
            }).catch(() => {});
        }

        async function duplicateWindow() {
            try {
                await fetch('/api/duplicate', { method: 'POST', body: '{}' });
            } catch(e) {}
        }

        async function quitApp() {
            saveDetailsState();
            try {
                await fetch('/api/quit', { 
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ win_count: 1 })
                });
            } catch (e) {
                console.error(e);
            }
        }

        async function exitWindow() {
            try {
                safeStorage.removeItem(`rizkyby_${windowId}_details`);
                safeStorage.removeItem(`rizkyby_${windowId}_font_size`);
                safeStorage.removeItem(`rizkyby_${windowId}_fullscreen`);
                safeStorage.removeItem(`rizkyby_${windowId}_mode`);
                safeStorage.removeItem(`rizkyby_${windowId}_palette_dark`);
                safeStorage.removeItem(`rizkyby_${windowId}_palette_light`);
                await fetch('/api/close_window?win=' + encodeURIComponent(windowId) + '&erase=1', { method: 'POST' });
                setTimeout(() => { window.close(); }, 200);
            } catch (e) {
                setTimeout(() => { window.close(); }, 300);
            }
        }

        let _isWindowOnTop = false;
        async function toggleAlwaysOnTop() {
            try {
                const res = await fetch('/api/on_top?win=' + encodeURIComponent(windowId), { method: 'POST' });
                if (res.ok) {
                    const data = await res.json();
                    _isWindowOnTop = data.on_top;

                    // Simpan ke storage lokal & server config
                    safeStorage.setItem(`rizkyby_${windowId}_on_top`, _isWindowOnTop ? 'true' : 'false');
                    saveConfig({ on_top: _isWindowOnTop });

                    const btn = document.getElementById('btn-app-duplicate');
                    if (btn) {
                        if (_isWindowOnTop) {
                            btn.classList.add('pinned');
                            btn.innerHTML = '🖈';
                            btn.style.color = 'var(--accent-orange)';
                            btn.style.textShadow = '0 0 10px var(--accent-orange)';
                            btn.setAttribute('data-tooltip', 'pinned');
                        } else {
                            btn.classList.remove('pinned');
                            btn.innerHTML = '🗗';
                            btn.style.color = 'var(--accent-blue)';
                            btn.style.textShadow = 'none';
                            btn.setAttribute('data-tooltip', 'duplicate');
                        }
                    }

                    if (_isWindowOnTop) {
                        showCenterToast('🖈 Always On Top: ENABLED (Pinned Above)');
                    } else {
                        showCenterToast('🗗 Always On Top: DISABLED (Unpinned)');
                    }
                }
            } catch(e) {}
        }

        // 1. Matikan icon autoscroll browser saat tombol tengah mouse ditekan
        document.addEventListener('mousedown', function(e) {
            if (e.button === 1) {
                e.preventDefault();
                e.stopPropagation();
            }
        }, { capture: true });

        // 2. Handler Copy to Clipboard dengan Klik Tengah
        document.addEventListener('auxclick', function(e) {
            if (e.button !== 1) return; // Khusus middle click

            // 1. Salin URL Repository jika tombol GitHub (asli maupun clone) diklik tengah
            const githubLinkEl = e.target.closest('.github-link') || e.target.closest('.easter-github-clone');
            if (githubLinkEl) {
                e.preventDefault();
                e.stopPropagation();
                // Blokir klik tengah jika tombol sedang meluncur
                if (isEasterTransitioning) return;

                const urlToCopy = githubLinkEl.getAttribute('href') || "https://github.com/rizkybayuu";
                copyText(githubLinkEl, urlToCopy, "Copied Link to Clipboard!");
                return;
            }

            e.preventDefault();
            e.stopPropagation();

            // 2. Salin info tooltip header jika judul diklik tengah
            const titleEl = e.target.closest('#header-app-title');
            if (titleEl) {
                const headerTooltipHtml = window.hardwareTooltips ? window.hardwareTooltips['header'] : null;
                let textToCopy = "";
                if (headerTooltipHtml) {
                    const tempDiv = document.createElement('div');
                    tempDiv.innerHTML = headerTooltipHtml;
                    textToCopy = tempDiv.innerText || tempDiv.textContent;
                } else {
                    textToCopy = titleEl.textContent;
                }
                if (textToCopy && textToCopy.trim()) {
                    copyText(titleEl, textToCopy.trim(), "Copied Header Info to Clipboard!");
                }
                return;
            }

            // 3. Tombol-tombol bersensor / ber-tooltip (Color, Mode, Suhu, Baterai, Duplicate, Quit, dsb)
            const tooltipEl = e.target.closest('[data-tooltip]') || window._lastHoveredTooltipEl;
            if (tooltipEl) {
                const key = tooltipEl.getAttribute('data-tooltip');
                const htmlContent = window.hardwareTooltips ? window.hardwareTooltips[key] : null;
                if (htmlContent) {
                    const tempDiv = document.createElement('div');
                    tempDiv.innerHTML = htmlContent;
                    const tooltipText = tempDiv.innerText || tempDiv.textContent;
                    if (tooltipText && tooltipText.trim()) {
                        copyText(tooltipEl, tooltipText.trim(), "Copied Tooltip to Clipboard!");
                        return;
                    }
                }
            }

            // 4. Salin teks kartu telemetri
            const card = e.target.closest('.card');
            if (card) {
                let textToCopy = "";
                const detailsEl = card.querySelector('.card-inner-details');
                if (card.classList.contains('show-details') && detailsEl && detailsEl.textContent) {
                    textToCopy = detailsEl.textContent;
                } else {
                    textToCopy = card.innerText || card.textContent;
                }
                if (textToCopy && textToCopy.trim()) {
                    copyText(card, textToCopy.trim(), "Copied Data to Clipboard!");
                }
            }
        }, { capture: true });

        // Zoom functionality
        let currentZoom = 16;
        document.addEventListener('wheel', function(e) {
            if (e.ctrlKey) {
                e.preventDefault();
                if (e.deltaY < 0) {
                    currentZoom += 1;
                } else {
                    currentZoom -= 1;
                }
                currentZoom = Math.max(8, Math.min(currentZoom, 32));
                document.documentElement.style.fontSize = currentZoom + 'px';
                resyncChartFonts();

                scrollCreditFrozenToBottom();

                if (isEasterEggActive) {
                    // Update posisi clone di titik 1
                    updateEasterEggPositions();
                    // Update koordinat target titik 0 sesuai skala zoom baru
                    recalcSavedPositionsOnZoom();
                }

                safeStorage.setItem(`rizkyby_${windowId}_font_size`, currentZoom);
                saveConfig({ font_size: currentZoom });
            }
        }, { passive: false });
    
        function closeCardDetails(btn) {
            const card = btn.closest('.card');
            if (card) {
                card.classList.remove('show-details');
                saveDetailsState();
            }
        }

        // Keyboard Shortcuts Manager
        document.addEventListener('keydown', (e) => {
            // Alt + S: Toggle Autoscroll Mode (Cycle ▶ ⏸ ■)
            if (e.altKey && e.key.toLowerCase() === 's') {
                e.preventDefault();
                cycleAutoscrollMode();
                return;
            }

            // Alt + F: Toggle Telemetry Refresh Rate Panel
            if (e.altKey && e.key.toLowerCase() === 'f') {
                e.preventDefault();
                openTelemetryIntervalPopup();
                return;
            }

            // Ctrl + Shift + A: Toggle Keep Above Other (Pin Window)
            if ((e.ctrlKey || e.metaKey) && e.shiftKey && e.key.toLowerCase() === 'a') {
                e.preventDefault();
                toggleAlwaysOnTop();
                return;
            }
            // Ctrl + Q: Quit All & Save Layout
            if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 'q') {
                e.preventDefault();
                quitApp();
                return;
            }
            // Alt + Q: Exit Current Window
            if (e.altKey && e.key.toLowerCase() === 'q') {
                e.preventDefault();
                exitWindow();
                return;
            }
            // Ctrl + N: Duplicate Window
            if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 'n') {
                e.preventDefault();
                duplicateWindow();
                return;
            }
        });

        // Fullscreen & Right-Click Details Logic
        const overlay = document.getElementById('fs-overlay');

        function saveDetailsState() {
            const activeCards = [];
            document.querySelectorAll('.card.show-details').forEach(c => {
                const cls = Array.from(c.classList).find(l => l.startsWith('card-'));
                if (cls) activeCards.push(cls);
            });
            safeStorage.setItem(`rizkyby_${windowId}_details`, activeCards.join(','));
            saveConfig({ details: activeCards });
        }

        function toggleCardDetails(card, e) {
            if (e) {
                if (e.target && e.target.closest('.disk-selector-wrapper, .gpu-selector-wrapper, #custom-disk-select, #custom-gpu-select, #custom-color-select, .custom-options-menu, .mode-toggle-group, #btn-app-quit, #btn-app-duplicate')) {
                    return;
                }
                e.preventDefault();
                e.stopPropagation();
            }
            const now = Date.now();
            if (card._lastToggle && (now - card._lastToggle < 200)) return;
            card._lastToggle = now;

            card.classList.toggle('show-details');

            // REKAM INGATAN DETIK ITU JUGA
            saveDetailsState();

            const detailsEl = card.querySelector('.card-inner-details');
            if (detailsEl) detailsEl.dataset.scrolling = "";
            setTimeout(applyAutoScroll, 50);
        }

        document.querySelectorAll('.card').forEach(card => {
            card.addEventListener('contextmenu', (e) => toggleCardDetails(card, e));
            card.addEventListener('mouseup', (e) => {
                if (e.button === 2) toggleCardDetails(card, e);
            });
            card.addEventListener('auxclick', (e) => {
                if (e.button === 2) toggleCardDetails(card, e);
            });
            
            card.addEventListener('click', (e) => {
                // Jangan picu fullscreen jika klik tombol aksi atau selector dropdown
                if (e.target && e.target.closest('.disk-selector-wrapper, #disk-select, .disk-select-dropdown, #custom-disk-select, #custom-gpu-select, #custom-color-select, #custom-color-options, .mode-toggle-group, #btn-app-quit, #btn-app-duplicate')) {
                    return;
                }

                // Pastikan target adalah elemen kartu terluar
                const targetCard = card.closest('.card');
                if (!targetCard) return;

                // BERSIHKAN TOTAL SEMUA INLINE MASK SAAT MASUK FULLSCREEN
                const dbEl = document.querySelector('.dashboard');
                if (dbEl && !isEasterEggActive) {
                    dbEl.style.webkitMaskImage = '';
                    dbEl.style.maskImage = '';
                }
                targetCard.style.webkitMaskImage = 'none';
                targetCard.style.maskImage = 'none';

                const isCurrentlyFs = targetCard.classList.contains('fullscreen');

                // Tutup semua kartu fullscreen lain terlebih dahulu
                document.querySelectorAll('.card.fullscreen').forEach(c => {
                    if (c !== targetCard) c.classList.remove('fullscreen');
                });

                if (isCurrentlyFs) {
                    targetCard.classList.remove('fullscreen');
                    overlay.classList.remove('active');
                    safeStorage.setItem(`rizkyby_${windowId}_fullscreen`, '');
                    saveConfig({ fullscreen: '' });
                } else {
                    targetCard.classList.add('fullscreen');
                    overlay.classList.add('active');
                    const cardClass = Array.from(targetCard.classList).find(c => c.startsWith('card-'));
                    if (cardClass) {
                        safeStorage.setItem(`rizkyby_${windowId}_fullscreen`, cardClass);
                        saveConfig({ fullscreen: cardClass });
                    }
                }

                resyncChartFonts();
                setTimeout(() => {
                    window.dispatchEvent(new Event('resize'));
                }, 50);
            });
        });
        
        overlay.addEventListener('click', () => {
            document.querySelectorAll('.card.fullscreen').forEach(c => c.classList.remove('fullscreen'));
            resyncChartFonts();
            overlay.classList.remove('active');
            safeStorage.setItem(`rizkyby_${windowId}_fullscreen`, '');
            saveConfig({ fullscreen: '' });
        });
        
        // Initialize Color Palette Selector
        initColorSelector();

        let isWheelThrottled = false;

        // Capture-phase wheel scroll shortcuts for Color Selector & Mode Toggle
        document.addEventListener('wheel', function(e) {
            const colorBox = e.target.closest('#custom-color-select, .custom-color-select-box, #custom-color-label');
            const isInsideMenu = e.target.closest('.custom-color-options-menu, .custom-disk-options-menu');
            if (colorBox && !isInsideMenu) {
                e.preventDefault();
                e.stopPropagation();
                if (isWheelThrottled) return;
                isWheelThrottled = true;
                setTimeout(() => { isWheelThrottled = false; }, 120);

                const palettes = getActivePalettes();
                const keys = Object.keys(palettes);
                let idx = keys.indexOf(currentPalette);
                if (idx === -1) idx = 0;

                if (e.deltaY > 0) {
                    idx = (idx + 1) % keys.length;
                } else {
                    idx = (idx - 1 + keys.length) % keys.length;
                }
                setPalette(keys[idx]);
                return;
            }

            const modeToggle = e.target.closest('.mode-toggle-group, #btn-mode-dark, #btn-mode-light');
            const isQuitOrDup = e.target.closest('#btn-app-quit, #btn-app-duplicate');
            if (modeToggle && !isQuitOrDup) {
                e.preventDefault();
                e.stopPropagation();
                if (isWheelThrottled) return;
                isWheelThrottled = true;
                setTimeout(() => { isWheelThrottled = false; }, 200);

                const nextMode = currentThemeMode === 'dark' ? 'light' : 'dark';
                setThemeMode(nextMode);
                return;
            }
        }, { passive: false, capture: true });

        // Immediately blur clicked elements to remove lingering focus boxes / outlines
        document.addEventListener('mouseup', function(e) {
            if (e.target && e.target.id === 'header-app-title') return; // Pengecualian agar judul tetap fokus
            if (e.target && typeof e.target.blur === 'function') {
                setTimeout(() => {
                    try { e.target.blur(); } catch(err) {}
                }, 50);
            }
        });

        async function restoreSavedState() {
            const btn = document.getElementById('btn-app-duplicate');

            // 1. CEK INSTAN DARI STORAGE LOKAL (Mencegah kedipan ikon saat awal launch)
            const localOnTop = safeStorage.getItem(`rizkyby_${windowId}_on_top`);
            if (localOnTop === 'true') {
                _isWindowOnTop = true;
                if (btn) {
                    btn.classList.add('pinned');
                    btn.innerHTML = '🖈';
                    btn.style.color = 'var(--accent-orange)';
                    btn.style.textShadow = '0 0 10px var(--accent-orange)';
                    btn.setAttribute('data-tooltip', 'pinned');
                }
            } else if (localOnTop === 'false') {
                _isWindowOnTop = false;
                if (btn) {
                    btn.classList.remove('pinned');
                    btn.innerHTML = '🗗';
                    btn.style.color = 'var(--accent-blue)';
                    btn.style.textShadow = 'none';
                    btn.setAttribute('data-tooltip', 'duplicate');
                }
            }

            // 2. AMBIL CONFIG DARI SERVER BACKEND
            let cfg = {};
            try {
                const res = await fetch('/api/config?win=' + encodeURIComponent(windowId));
                if (res.ok) cfg = await res.json();
            } catch(e) {}

            // Sinkronkan interval telemetri dari config server
            if (cfg.telemetry_interval) {
                const sec = cfg.telemetry_interval / 1000;
                if (sec >= 0.5) setTelemetryIntervalSeconds(sec, false);
            }

            // Sinkronkan status On-Top jika ada di config server
            const winState = (cfg.per_window_settings && cfg.per_window_settings[windowId]) ? cfg.per_window_settings[windowId] : cfg;
            const isPinned = (winState && winState.on_top !== undefined) ? winState.on_top : (_isWindowOnTop);

            _isWindowOnTop = !!isPinned;
            safeStorage.setItem(`rizkyby_${windowId}_on_top`, _isWindowOnTop ? 'true' : 'false');

            if (btn) {
                if (_isWindowOnTop) {
                    btn.classList.add('pinned');
                    btn.innerHTML = '🖈';
                    btn.style.color = 'var(--accent-orange)';
                    btn.style.textShadow = '0 0 10px var(--accent-orange)';
                    btn.setAttribute('data-tooltip', 'pinned');
                } else {
                    btn.classList.remove('pinned');
                    btn.innerHTML = '🗗';
                    btn.style.color = 'var(--accent-blue)';
                    btn.style.textShadow = 'none';
                    btn.setAttribute('data-tooltip', 'duplicate');
                }
            }

            // Restore right-click details mode per card
            let activeDetails = [];
            const localDetails = safeStorage.getItem(`rizkyby_${windowId}_details`);
            if (localDetails) {
                try {
                    const parsed = JSON.parse(localDetails);
                    if (Array.isArray(parsed)) activeDetails = parsed;
                    else if (typeof localDetails === 'string') activeDetails = localDetails.split(',');
                } catch(e) {
                    activeDetails = localDetails.split(',');
                }
            } else if (winState && winState.details) {
                if (Array.isArray(winState.details)) {
                    activeDetails = winState.details;
                } else if (typeof winState.details === 'string') {
                    try {
                        const parsed = JSON.parse(winState.details);
                        if (Array.isArray(parsed)) activeDetails = parsed;
                        else activeDetails = winState.details.split(',');
                    } catch(e) {
                        activeDetails = winState.details.split(',');
                    }
                }
            }

            document.querySelectorAll('.card').forEach(c => c.classList.remove('show-details'));
            if (activeDetails && activeDetails.length > 0) {
                activeDetails.forEach(cls => {
                    if (cls && typeof cls === 'string' && cls.trim()) {
                        const targetCard = document.querySelector('.' + cls.trim());
                        if (targetCard) {
                            targetCard.classList.add('show-details');
                        }
                    }
                });
            }

            // Restore font zoom size
            const savedZoom = cfg.font_size || safeStorage.getItem(`rizkyby_${windowId}_font_size`) || safeStorage.getItem('rizkyby_font_size');
            if (savedZoom) {
                currentZoom = parseInt(savedZoom, 10);
                document.documentElement.style.fontSize = currentZoom + 'px';
                resyncChartFonts();
            }

            // Restore Theme Mode (Dark/Light)
            if (cfg.palette_dark) safeStorage.setItem(`rizkyby_${windowId}_palette_dark`, cfg.palette_dark);
            if (cfg.palette_light) safeStorage.setItem(`rizkyby_${windowId}_palette_light`, cfg.palette_light);
            const savedMode = cfg.mode || safeStorage.getItem(`rizkyby_${windowId}_mode`) || 'dark';
            const activePalette = savedMode === 'light'
                ? (cfg.palette_light || safeStorage.getItem(`rizkyby_${windowId}_palette_light`) || 'breeze')
                : (cfg.palette_dark || safeStorage.getItem(`rizkyby_${windowId}_palette_dark`) || 'cyberpunk');
            setThemeMode(savedMode, activePalette);

            // Restore Fullscreen Card Mode
            const savedFsCard = cfg.fullscreen || safeStorage.getItem(`rizkyby_${windowId}_fullscreen`) || safeStorage.getItem('rizkyby_fullscreen');
            if (savedFsCard) {
                setTimeout(() => {
                    const targetCard = document.querySelector('.' + savedFsCard);
                    if (targetCard) {
                        document.querySelectorAll('.card.fullscreen').forEach(c => c.classList.remove('fullscreen'));
                        targetCard.classList.add('fullscreen');
                        resyncChartFonts();
                        const overlayEl = document.getElementById('fs-overlay');
                        if (overlayEl) overlayEl.classList.add('active');
                        window.dispatchEvent(new Event('resize'));
                    }
                }, 300);
            }
        }

        // Auto restore user's last saved state on launch
        restoreSavedState();

        // Handler Animasi & Scroll Panel About (1.25x Faster, 1s Ease-in & Smooth End)
        let aboutScrollTimer = null;
        let aboutScrollAnimId = null;
        let isAboutManualScrolling = false;
        let aboutManualResumeTimer = null;

        function stopAboutAutoScroll() {
            if (aboutScrollTimer) { clearTimeout(aboutScrollTimer); aboutScrollTimer = null; }
            if (aboutScrollAnimId) { cancelAnimationFrame(aboutScrollAnimId); aboutScrollAnimId = null; }
        }

        function startAboutCreditsScroll() {
            stopAboutAutoScroll();
            const panel = document.getElementById('about-panel');
            // Cegah auto-scroll jika panel About / Easter Egg sedang TIDAK aktif
            if (!panel || !document.body.classList.contains('show-about-panel')) return;

            isAboutManualScrolling = false;

            if (!panel._hasManualScrollListener) {
                panel._hasManualScrollListener = true;

                const handleManualScroll = (e) => {
                    isAboutManualScrolling = true;
                    if (aboutScrollAnimId) {
                        cancelAnimationFrame(aboutScrollAnimId);
                        aboutScrollAnimId = null;
                    }
                    if (aboutScrollTimer) {
                        clearTimeout(aboutScrollTimer);
                        aboutScrollTimer = null;
                    }

                    if (e && e.deltaY) {
                        panel.scrollTop += e.deltaY * 0.5;
                    }

                    if (aboutManualResumeTimer) clearTimeout(aboutManualResumeTimer);
                    aboutManualResumeTimer = setTimeout(() => {
                        isAboutManualScrolling = false;
                        resumeAboutCreditsScroll();
                    }, 4000);
                };

                panel.addEventListener('wheel', handleManualScroll, { passive: false });
                panel.addEventListener('touchmove', handleManualScroll, { passive: true });
            }

            // Langsung lanjutkan animasi dari posisi terakhimya
            resumeAboutCreditsScroll();
        }

        function resumeAboutCreditsScroll() {
            if (aboutScrollAnimId) cancelAnimationFrame(aboutScrollAnimId);
            const panel = document.getElementById('about-panel');
            if (!panel || isAboutManualScrolling || !isAutoscrollAllowed()) return;

            const maxScroll = panel.scrollHeight - panel.clientHeight;
            if (maxScroll <= 0) return;

            const startPos = panel.scrollTop;
            if (startPos >= maxScroll - 2) {
                fastScrollToTop(panel, () => {
                    startAboutCreditsScroll();
                });
                return;
            }

            // === PENGATURAN KECEPATAN & EASE ===
            const targetLinearSpeed = 65;
            const easeDuration = 1000;
            const initialDelay = 2000; // Jeda tepat 3 detik di titik 0 sebelum mulai meluncur

            const easeDistance = 0.5 * targetLinearSpeed * (easeDuration / 1000);
            const totalRemaining = maxScroll - startPos;
            const linearDistance = Math.max(0, totalRemaining - (2 * easeDistance));
            const linearDuration = (linearDistance / targetLinearSpeed) * 1000;
            const totalTripDuration = initialDelay + (2 * easeDuration) + linearDuration;

            let startTime = null;

            function scrollDownStep(timestamp) {
                if (isAboutManualScrolling) return;
                if (!startTime) startTime = timestamp;
                const elapsed = timestamp - startTime;

                let currentScroll = startPos;

                if (elapsed < initialDelay) {
                    currentScroll = startPos;
                } else if (elapsed < initialDelay + easeDuration) {
                    const t = (elapsed - initialDelay) / easeDuration;
                    currentScroll = startPos + (easeDistance * t * t);
                } else if (elapsed < initialDelay + easeDuration + linearDuration) {
                    const linearElapsed = (elapsed - initialDelay - easeDuration) / 1000;
                    currentScroll = startPos + easeDistance + (linearElapsed * targetLinearSpeed);
                } else if (elapsed < totalTripDuration) {
                    const t = (elapsed - initialDelay - easeDuration - linearDuration) / easeDuration;
                    const decelFactor = t * (2 - t);
                    currentScroll = startPos + easeDistance + linearDistance + (easeDistance * decelFactor);
                } else {
                    currentScroll = maxScroll;
                }

                if (currentScroll < maxScroll && elapsed < totalTripDuration) {
                    panel.scrollTop = currentScroll;
                    aboutScrollAnimId = requestAnimationFrame(scrollDownStep);
                } else {
                    panel.scrollTop = maxScroll;
                    // Tepat 2 detik di ujung bawah (titik 1) sebelum kembali ke atas
                    aboutScrollTimer = setTimeout(() => {
                        fastScrollToTop(panel, () => {
                            startAboutCreditsScroll();
                        });
                    }, 2000);
                }
            }

            aboutScrollAnimId = requestAnimationFrame(scrollDownStep);
        }

        function fastScrollToTop(panel, onComplete) {
            const startPos = panel.scrollTop;
            const duration = 700;
            let startTime = null;
            const easeInOutQuad = t => t < 0.5 ? 2 * t * t : -1 + (4 - 2 * t) * t;

            function scrollUpStep(timestamp) {
                if (!startTime) startTime = timestamp;
                const elapsed = timestamp - startTime;
                const t = Math.min(1, elapsed / duration);
                panel.scrollTop = startPos * (1 - easeInOutQuad(t));

                if (t < 1) {
                    aboutScrollAnimId = requestAnimationFrame(scrollUpStep);
                } else {
                    panel.scrollTop = 0;
                    if (onComplete) onComplete();
                }
            }
            aboutScrollAnimId = requestAnimationFrame(scrollUpStep);
        }

        // =================================================================
        // FUNGSI UPDATE ISI EASTER EGG (Bisa dipanggil dari mana saja)
        // =================================================================
        let easterEggAnimTimeout = null;

        function openExternalLink(url) {
            if (!url) return;
            // Ditangkap oleh on_decide_policy di C++ dan dieksekusi via xdg-open
            window.location.href = url;
        }

        function updateEasterEggContent(isAboutOpen) {
            const container = document.getElementById('easter-credit-container');
            if (!container) return;
            // Ambil elemen div bagian dalam yang sedang aktif
            const innerDiv = container.firstElementChild;

                        // Sub-fungsi untuk merender HTML dengan animasi IN
            const renderHTML = () => {
                // 1. Reset posisi scroll ke atas
                container.scrollTop = 0;

                if (isAboutOpen) {
                    // MODE: KREDIT KEDUA (Special Thanks)
                    container.innerHTML = `
                        <div style="animation: cardsMoveUpIn 0.4s ease-out forwards; max-width: 760px; width: 100%; padding: 0.8rem 1.5rem 8rem 1.5rem; display: flex; flex-direction: column; align-items: center; text-align: center; gap: 1.8rem;">
                            <div style="font-size: clamp(1.4rem, 2.2vw, 2rem); font-weight: 800; color: var(--accent-cyan); letter-spacing: 1.5px; margin-top: 0.2rem;">
                                SYSTEM ARCHITECTURE &amp; ACKNOWLEDGMENTS
                            </div>

                            <div class="tech-pill-group">
                                <span class="tech-pill" style="border-color: var(--accent-purple); color: var(--accent-purple);">Cross-Platform Core</span>
                                <span class="tech-pill" style="border-color: var(--accent-blue); color: var(--accent-blue);">Native C++ Subsystem</span>
                                <span class="tech-pill" style="border-color: var(--accent-green); color: var(--accent-green);">Real-time Telemetry</span>
                            </div>

                            <div style="font-size: 1rem; color: var(--text-main); line-height: 1.8; max-width: 42rem;">
                                RizkybyMONITOR is engineered as a responsive, lightweight system diagnostic suite. It utilizes a decoupled native backend architecture to stream real-time hardware telemetry with minimal performance overhead.
                            </div>

                            <!-- SECTION 1: DEVELOPER & ARCHITECTURE -->
                            <div class="about-section" style="width: 100%; gap: 1.2rem;">
                                <div class="about-section-heading" style="color: var(--accent-purple);">Core Developer &amp; Architecture Model</div>
                                <div class="about-spec-list">
                                    <div class="about-spec-row">
                                        <span class="about-spec-label">Project Lead &amp; Creator</span>
                                        <span class="about-spec-value">Rizky Bayuu</span>
                                    </div>
                                    <div class="about-spec-row">
                                        <span class="about-spec-label">Architecture Pattern</span>
                                        <span class="about-spec-value">Decoupled Native Backend + Embedded WebView UI</span>
                                    </div>
                                    <div class="about-spec-row">
                                        <span class="about-spec-label">Execution Loop</span>
                                        <span class="about-spec-value">Asynchronous Multi-Threaded Telemetry Pipeline</span>
                                    </div>
                                    <div class="about-spec-row">
                                        <span class="about-spec-label">State Management</span>
                                        <span class="about-spec-value">Local Storage Persistence &amp; Dynamic IPC Bridge</span>
                                    </div>
                                </div>
                            </div>

                            <!-- SECTION 2: STACK & FRAMEWORK -->
                            <div class="about-section" style="width: 100%; gap: 1.2rem;">
                                <div class="about-section-heading" style="color: var(--accent-blue);">Technology Stack &amp; Open Source Libraries</div>
                                <div class="about-spec-list">
                                    <div class="about-spec-row">
                                        <span class="about-spec-label">Core System Logic</span>
                                        <span class="about-spec-value">ISO C++17 Standard Subsystem</span>
                                    </div>
                                    <div class="about-spec-row">
                                        <span class="about-spec-label">Interface Rendering</span>
                                        <span class="about-spec-value">HTML5, CSS3 Design Tokens &amp; Vanilla JavaScript</span>
                                    </div>
                                    <div class="about-spec-row">
                                        <span class="about-spec-label">Data Visualization</span>
                                        <span class="about-spec-value">Chart.js HTML5 Canvas Rendering Engine</span>
                                    </div>
                                    <div class="about-spec-row">
                                        <span class="about-spec-label">Typography System</span>
                                        <span class="about-spec-value">Google Outfit &amp; Inter Variable Fonts</span>
                                    </div>
                                </div>
                            </div>

                            <!-- SECTION 3: DESIGN PRINCIPLES -->
                            <div class="about-section" style="width: 100%; gap: 1.2rem;">
                                <div class="about-section-heading" style="color: var(--accent-green);">Design Standards &amp; Principles</div>
                                <div class="about-spec-list">
                                    <div class="about-spec-row">
                                        <span class="about-spec-label">Performance Target</span>
                                        <span class="about-spec-value">Low Overhead, Sub-Millisecond UI Render Cycle</span>
                                    </div>
                                    <div class="about-spec-row">
                                        <span class="about-spec-label">Window Container</span>
                                        <span class="about-spec-value">Custom Frameless Window with Integrated Drag Controls</span>
                                    </div>
                                    <div class="about-spec-row">
                                        <span class="about-spec-label">Color Engine</span>
                                        <span class="about-spec-value">Dynamic Palette Matrix (Independent Session Themes)</span>
                                    </div>
                                </div>
                            </div>

                            <p class="closing-note" style="color: var(--text-muted);">
                                Thank you for using RizkybyMONITOR.
                            </p>
                        </div>
                    `;
                    startEasterCreditsScroll();
                } else {
                    // MODE: CHANGELOG (Judul Bernafas Rapi, Card Melar di Tengah, Deskripsi Bawah Terkunci)
                    container.innerHTML = `
                        <div style="animation: cardsMoveUpIn 0.4s ease-out forwards; width: 100vw; height: 100%; padding: 0.6rem 0 1.2rem 0; box-sizing: border-box; display: flex; flex-direction: column; align-items: center; text-align: center; gap: 0.6rem; overflow: hidden;">

                            <!-- JUDUL (Rapat dan presisi di bawah tombol GitHub) -->
                            <div style="font-size: clamp(1.35rem, 2vw, 1.9rem); font-weight: 800; color: var(--accent-orange); letter-spacing: 1.5px; flex-shrink: 0; margin-top: 0.1rem; margin-bottom: 0.2rem;">
                                SYSTEM RELEASE CHANGELOG
                            </div>

                            <!-- WADAH CARD HORISONTAL -->
                            <div id="changelog-cards-wrapper" class="changelog-cards-container">
                                <!-- Card 1 dan Card 2 tetap sama -->
                                <div class="changelog-card" style="text-align: left;">
                                    <div style="display: flex; flex-direction: column; gap: 0.3rem; padding-bottom: 0.6rem; border-bottom: 1px solid rgba(255,255,255,0.12); flex-shrink: 0;">
                                        <div style="display: flex; justify-content: space-between; align-items: center;">
                                            <span style="font-size: 1.15rem; font-weight: 800; color: var(--accent-blue);">v1.0 Base</span>
                                            <span class="tech-pill" style="border-color: var(--accent-blue); color: var(--accent-blue); font-size: 0.72rem;">Initial Release</span>
                                        </div>
                                        <div style="font-size: 0.8rem; color: var(--text-muted);">Foundational architecture &amp; core telemetry engine.</div>
                                    </div>

                                    <div class="changelog-card-body">
                                        <div class="changelog-item">
                                            <span class="changelog-item-num">01.</span>
                                            <div class="changelog-item-content">
                                                <div class="changelog-item-title">Native C++17 Core Engine</div>
                                                <div class="changelog-item-desc">Engineered as a zero-dependency single binary build using ISO C++17 standard with sub-10ms startup (&lt; 25MB RAM).</div>
                                            </div>
                                        </div>

                                        <div class="changelog-item">
                                            <span class="changelog-item-num">02.</span>
                                            <div class="changelog-item-content">
                                                <div class="changelog-item-title">Frameless Multi-Window Management</div>
                                                <div class="changelog-item-desc">Borderless container with native dragging (Super+Drag), multi-window instantiation (Ctrl+N), and Always-On-Top pinning (Ctrl+Shift+A).</div>
                                            </div>
                                        </div>

                                        <div class="changelog-item">
                                            <span class="changelog-item-num">03.</span>
                                            <div class="changelog-item-content">
                                                <div class="changelog-item-title">Per-Window State Persistence</div>
                                                <div class="changelog-item-desc">Independent state management in config.json storing window coordinates, dimensions, selected disk, theme, and font scale.</div>
                                            </div>
                                        </div>

                                        <div class="changelog-item">
                                            <span class="changelog-item-num">04.</span>
                                            <div class="changelog-item-content">
                                                <div class="changelog-item-title">Hardware Telemetry Pipeline</div>
                                                <div class="changelog-item-desc">Live CPU usage %, frequencies, Intel Iris Xe 4-engine GPU cluster, Physical RAM, Buffers, and Swap partition tracking.</div>
                                            </div>
                                        </div>

                                        <div class="changelog-item">
                                            <span class="changelog-item-num">05.</span>
                                            <div class="changelog-item-content">
                                                <div class="changelog-item-title">SMART Storage Health &amp; Diagnostics</div>
                                                <div class="changelog-item-desc">Hardware SMART telemetry for NVMe, SATA SSD, HDD, and Flash drives tracking PASSED status, temperature, and TBW lifespan.</div>
                                            </div>
                                        </div>

                                        <div class="changelog-item">
                                            <span class="changelog-item-num">06.</span>
                                            <div class="changelog-item-content">
                                                <div class="changelog-item-title">Interactive CLI Diagnostic Mode</div>
                                                <div class="changelog-item-desc">Right-click toggle on widget cards (Disk &amp; Memory) to display formatted CLI diagnostic logs (free -h, lsblk, smartctl).</div>
                                            </div>
                                        </div>

                                        <div class="changelog-item">
                                            <span class="changelog-item-num">07.</span>
                                            <div class="changelog-item-content">
                                                <div class="changelog-item-title">Dual Themes &amp; Color Palettes</div>
                                                <div class="changelog-item-desc">Instant Dark Mode &amp; Light Mode switchers paired with 7 handcrafted ambient color palettes supporting mouse wheel cycle.</div>
                                            </div>
                                        </div>

                                        <div class="changelog-item">
                                            <span class="changelog-item-num">08.</span>
                                            <div class="changelog-item-content">
                                                <div class="changelog-item-title">Clipboard &amp; Tooltip Integration</div>
                                                <div class="changelog-item-desc">Middle-click any card or hover tooltip to copy telemetry metrics directly to system clipboard.</div>
                                            </div>
                                        </div>
                                    </div>
                                </div>

                                <!-- CARD 2: VERSION 1.1 STABLE OVERHAUL -->
                                <div class="changelog-card" style="text-align: left;">
                                    <div style="display: flex; flex-direction: column; gap: 0.3rem; padding-bottom: 0.6rem; border-bottom: 1px solid rgba(255,255,255,0.12); flex-shrink: 0;">
                                        <div style="display: flex; justify-content: space-between; align-items: center;">
                                            <span style="font-size: 1.15rem; font-weight: 800; color: var(--accent-orange);">v1.1 Stable</span>
                                            <span class="tech-pill" style="border-color: var(--accent-orange); color: var(--accent-orange); font-size: 0.72rem;">Milestone Stable</span>
                                        </div>
                                        <div style="font-size: 0.8rem; color: var(--text-muted);">Major performance, memory topology &amp; multi-GPU update.</div>
                                    </div>

                                    <div class="changelog-card-body">
                                        <div class="changelog-item">
                                            <span class="changelog-item-num">01.</span>
                                            <div class="changelog-item-content">
                                                <div class="changelog-item-title">Total Cross-Platform Parity (Linux &amp; Windows)</div>
                                                <div class="changelog-item-desc">Eliminated 14 architectural gaps between OS platforms, unifying Win32/WebView2 and GTK3/WebKit2GTK native subsystems.</div>
                                            </div>
                                        </div>

                                        <div class="changelog-item">
                                            <span class="changelog-item-num">02.</span>
                                            <div class="changelog-item-content">
                                                <div class="changelog-item-title">Unified 1-Second Real-Time Telemetry Sync</div>
                                                <div class="changelog-item-desc">Overhauled asynchronous polling engine into a strict non-blocking 1000ms loop across CPU, GPU, Memory, Network, and Disk cards without delay.</div>
                                            </div>
                                        </div>

                                        <div class="changelog-item">
                                            <span class="changelog-item-num">03.</span>
                                            <div class="changelog-item-content">
                                                <div class="changelog-item-title">Multi-Tier Hierarchical Memory Topology</div>
                                                <div class="changelog-item-desc">Speed-classified memory pipeline tracking CPU Smart Cache (~1.5 TB/s), Dedicated VRAM, Physical RAM (Apps/Buffers/Shared), ZRAM (~20 GB/s), and SWAP.</div>
                                            </div>
                                        </div>

                                        <div class="changelog-item">
                                            <span class="changelog-item-num">04.</span>
                                            <div class="changelog-item-content">
                                                <div class="changelog-item-title">Dynamic Multi-GPU Cluster &amp; Engine Translation</div>
                                                <div class="changelog-item-desc">Integrated GPU device selector dropdown for iGPU (Intel/AMD) &amp; eGPU (NVIDIA/AMD) with dynamic CUDA/Tensor Engine label translation.</div>
                                            </div>
                                        </div>

                                        <div class="changelog-item">
                                            <span class="changelog-item-num">05.</span>
                                            <div class="changelog-item-content">
                                                <div class="changelog-item-title">CPU Hybrid P-Core &amp; E-Core Topology Engine</div>
                                                <div class="changelog-item-desc">Re-architected CPU card into 2:1 ratio with dynamic getDynamicCpuTopology detecting Performance Cores vs Efficiency Cores and live per-core clocks.</div>
                                            </div>
                                        </div>

                                        <div class="changelog-item">
                                            <span class="changelog-item-num">06.</span>
                                            <div class="changelog-item-content">
                                                <div class="changelog-item-title">All-Card Detailed CLI Text Diagnostic Mode</div>
                                                <div class="changelog-item-desc">Expanded right-click CLI text mode across ALL 5 widget cards (CPU, GPU, Memory, Network, Disk) with full raw diagnostic log streaming.</div>
                                            </div>
                                        </div>

                                        <div class="changelog-item">
                                            <span class="changelog-item-num">07.</span>
                                            <div class="changelog-item-content">
                                                <div class="changelog-item-title">SMART Fallback &amp; Dynamic USB Bridge Fix</div>
                                                <div class="changelog-item-desc">Eliminated hardcoded USB bridge flags (-d sntasmedia), fixed fake fallback values (42 °C / PASSED), and introduced strict 'N/A' error states.</div>
                                            </div>
                                        </div>

                                        <div class="changelog-item">
                                            <span class="changelog-item-num">08.</span>
                                            <div class="changelog-item-content">
                                                <div class="changelog-item-title">Multi-Window ID Gap Recovery Engine</div>
                                                <div class="changelog-item-desc">Re-architected multi-window restoration to parse exact active window IDs from config.json, fixing position mismatch when middle windows exit.</div>
                                            </div>
                                        </div>

                                        <div class="changelog-item">
                                            <span class="changelog-item-num">09.</span>
                                            <div class="changelog-item-content">
                                                <div class="changelog-item-title">Dynamic Network Adapter &amp; Signal Detection</div>
                                                <div class="changelog-item-desc">Replaced hardcoded Wi-Fi text with dynamic setDynamicNetworkTypeString detecting Wi-Fi SSIDs, signal %, Ethernet link speed, and gateways.</div>
                                            </div>
                                        </div>

                                        <div class="changelog-item">
                                            <span class="changelog-item-num">10.</span>
                                            <div class="changelog-item-content">
                                                <div class="changelog-item-title">Per-Device Selection State Memory</div>
                                                <div class="changelog-item-desc">Enhanced config.json persistence remembering active GPU choice and Disk I/O selection per window session across app restarts.</div>
                                            </div>
                                        </div>

                                        <div class="changelog-item">
                                            <span class="changelog-item-num">11.</span>
                                            <div class="changelog-item-content">
                                                <div class="changelog-item-title">Fullscreen Polish &amp; Typography Artifact Fixes</div>
                                                <div class="changelog-item-desc">Resolved middle-click clipboard copy on Wayland/X11, light mode text shadow artifacts, 36px window corner radius, and title tooltip hitboxes.</div>
                                            </div>
                                        </div>
                                    </div>
                                </div>

                                <!-- CARD 3: VERSION 1.2 STABLE (CURRENT PRODUCTION) -->
                                <div class="changelog-card" style="text-align: left;">
                                    <div style="display: flex; flex-direction: column; gap: 0.3rem; padding-bottom: 0.6rem; border-bottom: 1px solid rgba(255,255,255,0.12); flex-shrink: 0;">
                                        <div style="display: flex; justify-content: space-between; align-items: center;">
                                            <span style="font-size: 1.15rem; font-weight: 800; color: var(--accent-green, #10b981);">v1.2 Stable & Refined</span>
                                            <span class="tech-pill" style="border-color: var(--accent-green, #10b981); color: var(--accent-green, #10b981); font-size: 0.72rem;">Current Production</span>
                                        </div>
                                        <div style="font-size: 0.8rem; color: var(--text-muted);">Smart autoscroll engine, live telemetry panel &amp; CPU thermal helper.</div>
                                    </div>

                                    <div class="changelog-card-body">
                                        <div class="changelog-item">
                                            <span class="changelog-item-num">01.</span>
                                            <div class="changelog-item-content">
                                                <div class="changelog-item-title">Tri-State Smart Autoscroll Engine (▶ ⏸ ■)</div>
                                                <div class="changelog-item-desc">Introduced multi-mode global autoscroll (Always Active, Smart Focus/Hover Interaction, and Full Disabled for GPU power saving) with Alt+S toggle shortcut.</div>
                                            </div>
                                        </div>

                                        <div class="changelog-item">
                                            <span class="changelog-item-num">02.</span>
                                            <div class="changelog-item-content">
                                                <div class="changelog-item-title">Interactive Inactive Window Autoscroll</div>
                                                <div class="changelog-item-desc">Engineered smart dual-condition autoscroll: runs on active window, or on disabled windows upon mouse wheel/click interaction and pauses on mouseleave.</div>
                                            </div>
                                        </div>

                                        <div class="changelog-item">
                                            <span class="changelog-item-num">03.</span>
                                            <div class="changelog-item-content">
                                                <div class="changelog-item-title">Windows Super Key (Win Key) Stuck/Hold State Fix</div>
                                                <div class="changelog-item-desc">Eliminated low-level keyboard hook swallow bug causing Super key to freeze down in OS state. Implemented harmless dummy keystroke masking to block Start Menu without sticking.</div>
                                            </div>
                                        </div>

                                        <div class="changelog-item">
                                            <span class="changelog-item-num">04.</span>
                                            <div class="changelog-item-content">
                                                <div class="changelog-item-title">Adaptive Telemetry Refresh Glassmorphism Panel &amp; Alt+F</div>
                                                <div class="changelog-item-desc">Frosted glassmorphism telemetry panel with mouse wheel interval adjustment (min 500ms), glow styling, theme adaptation, and Alt+F shortcut.</div>
                                            </div>
                                        </div>

                                        <div class="changelog-item">
                                            <span class="changelog-item-num">05.</span>
                                            <div class="changelog-item-content">
                                                <div class="changelog-item-title">Dedicated CPU Thermal Sensor Helper (.NET 8)</div>
                                                <div class="changelog-item-desc">Integrated self-contained LibreHardwareMonitor sensor helper (rzkmon_sensor.exe) with 2s cache TTL delivering accurate real-time CPU Package &amp; Core thermal telemetry.</div>
                                            </div>
                                        </div>

                                        <div class="changelog-item">
                                            <span class="changelog-item-num">06.</span>
                                            <div class="changelog-item-content">
                                                <div class="changelog-item-title">Synchronized Dual-Axis Easter Egg Autoscroll</div>
                                                <div class="changelog-item-desc">Unified autoscroll lifecycle across Easter egg cards (horizontal wrapper) and card-body version text (vertical scroll) with smart pause/resume synchronization.</div>
                                            </div>
                                        </div>

                                        <div class="changelog-item">
                                            <span class="changelog-item-num">07.</span>
                                            <div class="changelog-item-content">
                                                <div class="changelog-item-title">Dynamic Interval &amp; Multi-Window Config API</div>
                                                <div class="changelog-item-desc">Restructured saveWindowConfig() to persist telemetry refresh rates via POST /api/config, seamlessly syncing user intervals across Win32 &amp; Linux GTK3.</div>
                                            </div>
                                        </div>

                                        <div class="changelog-item">
                                            <span class="changelog-item-num">08.</span>
                                            <div class="changelog-item-content">
                                                <div class="changelog-item-title">Automated Builder &amp; Package Manager Resilience</div>
                                                <div class="changelog-item-desc">Fixed CMD parenthesis syntax crash, added forced winget resolution (--force), direct %ProgramFiles% detection, and automated Microsoft bootstrap fallback.</div>
                                            </div>
                                        </div>

                                        <div class="changelog-item">
                                            <span class="changelog-item-num">09.</span>
                                            <div class="changelog-item-content">
                                                <div class="changelog-item-title">Multi-Window Positional Cascading &amp; C++ Headers</div>
                                                <div class="changelog-item-desc">Restored pws_pos multi-window coordinate cascading in wWinMain and added std::atomic header to guarantee clean cross-platform C++17 compilation.</div>
                                            </div>
                                        </div>

                                        <div class="changelog-item">
                                            <span class="changelog-item-num">10.</span>
                                            <div class="changelog-item-content">
                                                <div class="changelog-item-title">Tooltip Whitespace Indentation &amp; Multilingual Polish</div>
                                                <div class="changelog-item-desc">Standardized non-breaking spacing (&amp;nbsp;) on status symbols and eliminated redundant Indonesian/English text duplicates for consistent English UI.</div>
                                            </div>
                                        </div>
                                    </div>
                                </div>

                            </div>

                            <!-- 3. BAGIAN BAWAH: DESKRIPSI NYANTOL DI ALIGNMENT PALING BAWAH -->
                            <p class="closing-note" style="color: var(--text-muted); font-size: 0.8rem; margin: 0; padding: 0.2rem 0 0 0; flex-shrink: 0;">
                                All telemetry pipelines running on non-blocking 1000ms execution loop.
                            </p>
                        </div>
                    `;
                    // Cek jumlah kartu: Jika >= 4 kartu, ubah alignment ke kanan
                    const wrapper = document.getElementById('changelog-cards-wrapper');
                    if (wrapper) {
                        const totalCards = wrapper.querySelectorAll('.changelog-card').length;
                        if (totalCards >= 4) {
                            wrapper.classList.add('overflow-right');
                        } else {
                            wrapper.classList.remove('overflow-right');
                        }

                        // KUNCI: Kunci posisi horizontal awal ke posisi MENTOK AKHIR KANAN
                        requestAnimationFrame(() => {
                            const maxEnd = wrapper.scrollWidth - wrapper.clientWidth;
                            if (maxEnd > 0) {
                                wrapper.scrollLeft = maxEnd; // Langsung di posisi akhir
                            }
                        });
                    }

                    if (typeof applyAutoScroll === 'function') applyAutoScroll();
                    startChangelogHorizontalScroll(true); // Kirim flag bahwa ia dimulai dari posisi akhir
                }
            }; // <--- Tutup renderHTML di sini

            // Logika Eksekusi Animasi
            if (innerDiv) {
                innerDiv.style.animation = "cardsMoveUpOut 0.3s ease-in forwards";
                if (easterEggAnimTimeout) clearTimeout(easterEggAnimTimeout);
                easterEggAnimTimeout = setTimeout(renderHTML, 300);
            } else {
                renderHTML();
            }
        } // <--- Tutup updateEasterEggContent di sini

        // =================================================================
        // FUNGSI AUTO-SCROLL VERTIKAL UNTUK ISI CARD CHANGELOG
        // =================================================================
        let changelogCardScrollAnimId = null;

        // =====================================================================
        // MESIN AUTOSCROLL HORIZONTAL & VERTIKAL CHANGELOG TERPADU
        // =====================================================================
        // =====================================================================
        // MESIN AUTOSCROLL HORIZONTAL & VERTIKAL CHANGELOG TERPADU
        // =====================================================================
        let changelogHScrollAnimId = null;
        let changelogHScrollTimer = null;
        let isChangelogManualScrolling = false;
        let changelogManualResumeTimer = null;

        function stopChangelogHScroll(immediate = false) {
            if (changelogHScrollTimer) { clearTimeout(changelogHScrollTimer); changelogHScrollTimer = null; }
            if (changelogHScrollAnimId) { cancelAnimationFrame(changelogHScrollAnimId); changelogHScrollAnimId = null; }
        }

        // Kontrol autoscroll vertikal teks bernomor dalam card
        function stopCardVerticalScroll(cardBody) {
            if (cardBody._vAnimId) {
                cancelAnimationFrame(cardBody._vAnimId);
                cardBody._vAnimId = null;
            }
        }

        function runCardVerticalScroll(cardBody) {
            stopCardVerticalScroll(cardBody);
            if (cardBody._isHovered || cardBody._isManualScrolling || !document.getElementById('easter-credit-container') || !isAutoscrollAllowed()) return;

            const easeInOutCubic = t => t < 0.5 ? 4 * t * t * t : 1 - Math.pow(-2 * t + 2, 3) / 2;
            const inverseEaseInOutCubic = r => r <= 0.5 ? Math.cbrt(r / 4) : 1 - Math.cbrt((1 - r) * 2) / 2;

            let scrollMax = cardBody.scrollHeight - cardBody.clientHeight;
            if (scrollMax <= 0) return;

            const duration = Math.max(6000, Math.min(25000, 4000 + scrollMax * 35));

            let currentPos = Math.max(0, Math.min(scrollMax, cardBody.scrollTop));
            let ratio = currentPos / scrollMax;
            let linearRatio = inverseEaseInOutCubic(ratio);
            let dir = (cardBody._scrollDir !== undefined) ? cardBody._scrollDir : 1;
            let cycle = (dir === 1) ? linearRatio : (2 - linearRatio);

            let startTime = performance.now() - (cycle * duration);

            function step(currentTime) {
                if (cardBody._isHovered || cardBody._isManualScrolling || !document.getElementById('easter-credit-container') || !isAutoscrollAllowed()) {
                    cardBody._vAnimId = null;
                    return;
                }

                scrollMax = cardBody.scrollHeight - cardBody.clientHeight;
                if (scrollMax <= 0) return;

                let elapsed = currentTime - startTime;
                let curCycle = (elapsed / duration) % 2;
                cardBody._scrollDir = (curCycle <= 1) ? 1 : -1;

                let lRatio = (curCycle <= 1) ? curCycle : (2 - curCycle);
                let easedRatio = easeInOutCubic(lRatio);

                cardBody.scrollTop = easedRatio * scrollMax;
                cardBody._vAnimId = requestAnimationFrame(step);
            }
            cardBody._vAnimId = requestAnimationFrame(step);
        }

        function startChangelogHorizontalScroll(fromEnd = false) {
            stopChangelogHScroll();
            const panel = document.getElementById('changelog-cards-wrapper');
            if (!panel || !document.getElementById('easter-credit-container')) return;

            isChangelogManualScrolling = false;

            // Inisialisasi event hover & manual scroll vertikal di dalam kartu
            panel.querySelectorAll('.changelog-card-body').forEach(cardBody => {
                if (!cardBody._hasHoverSetup) {
                    cardBody._hasHoverSetup = true;

                    // Saat hover: Matikan autoscroll horizontal agar fokus membaca isi
                    cardBody.addEventListener('mouseenter', () => {
                        cardBody._isHovered = true;
                        stopChangelogHScroll();
                        stopCardVerticalScroll(cardBody);
                    });

                    // Saat mouse keluar: Lanjutkan autoscroll setelah jeda 1 detik
                    cardBody.addEventListener('mouseleave', () => {
                        cardBody._isHovered = false;
                        cardBody._isManualScrolling = false;
                        runCardVerticalScroll(cardBody);

                        if (changelogManualResumeTimer) clearTimeout(changelogManualResumeTimer);
                        changelogManualResumeTimer = setTimeout(() => {
                            resumeChangelogHScroll();
                        }, 1000);
                    });

                    // Manual scroll vertikal teks bernomor
                    cardBody.addEventListener('wheel', (e) => {
                        e.stopPropagation();
                        cardBody._isManualScrolling = true;
                        stopCardVerticalScroll(cardBody);
                        stopChangelogHScroll();

                        cardBody.scrollTop += e.deltaY;
                        cardBody._scrollDir = (e.deltaY >= 0) ? 1 : -1;

                        if (cardBody._manualVTimer) clearTimeout(cardBody._manualVTimer);
                        cardBody._manualVTimer = setTimeout(() => {
                            cardBody._isManualScrolling = false;
                            if (!cardBody._isHovered) {
                                runCardVerticalScroll(cardBody);
                            }
                        }, 1000);
                    }, { passive: false });
                }
            });

            // Picu langsung autoscroll vertikal isi kartu tanpa perlu menunggu manual wheel
            if (isAutoscrollAllowed()) {
                panel.querySelectorAll('.changelog-card-body').forEach(cardBody => {
                    runCardVerticalScroll(cardBody);
                });
            }

            // Wheel manual pada wadah luar kartu
            if (!panel._hasManualHScrollListener) {
                panel._hasManualHScrollListener = true;

                panel.addEventListener('wheel', (e) => {
                    if (e.target.closest('.changelog-card-body')) return;

                    isChangelogManualScrolling = true;
                    stopChangelogHScroll();

                    const delta = (Math.abs(e.deltaY) > Math.abs(e.deltaX)) ? e.deltaY : e.deltaX;
                    if (delta) panel.scrollLeft += delta * 0.6;

                    if (changelogManualResumeTimer) clearTimeout(changelogManualResumeTimer);
                    changelogManualResumeTimer = setTimeout(() => {
                        isChangelogManualScrolling = false;
                        resumeChangelogHScroll();
                    }, 1000);
                }, { passive: false });
            }

            // Saat animasi in muncul: Kunci posisi scroll di titik 1 (ujung kanan)
            const max = panel.scrollWidth - panel.clientWidth;
            if (fromEnd && max > 0) {
                panel.scrollLeft = max;
                // Diam 1 detik di titik 1, lalu fast scroll ke 0
                changelogHScrollTimer = setTimeout(() => {
                    fastScrollHToLeft(panel, () => {
                        resumeChangelogHScroll();
                    });
                }, 1000);
                return;
            }

            resumeChangelogHScroll();
        }

        // MESIN AUTOSCROLL 0 KE 1 (KIRI KE KANAN PERSIS SIFAT CREDIT)
        function resumeChangelogHScroll() {
            if (changelogHScrollAnimId) cancelAnimationFrame(changelogHScrollAnimId);
            const panel = document.getElementById('changelog-cards-wrapper');
            if (!panel || isChangelogManualScrolling || !document.getElementById('easter-credit-container') || !isAutoscrollAllowed()) return;

            const maxScroll = panel.scrollWidth - panel.clientWidth;
            if (maxScroll <= 0) return;

            const startPos = panel.scrollLeft;
            if (startPos >= maxScroll - 2) {
                changelogHScrollTimer = setTimeout(() => {
                    fastScrollHToLeft(panel, () => {
                        resumeChangelogHScroll();
                    });
                }, 1000); // Jeda 1 detik di titik 1 sebelum kembali ke 0
                return;
            }

            const initialDelay = 1000;    // Jeda 1 detik di titik 0
            const easeInTime = 1200;      // Akselerasi awal
            const easeOutTime = 1400;     // Deselerasi halus sampai diam
            const totalRemaining = maxScroll - startPos;
            const targetLinearSpeed = 65; // Kecepatan stabil (px/detik)

            const distEaseIn = 0.5 * targetLinearSpeed * (easeInTime / 1000);
            const distEaseOut = 0.5 * targetLinearSpeed * (easeOutTime / 1000);

            let linearDist = totalRemaining - (distEaseIn + distEaseOut);
            let linearTime = 0;
            if (linearDist > 0) {
                linearTime = (linearDist / targetLinearSpeed) * 1000;
            } else {
                linearDist = 0;
            }

            const totalDuration = initialDelay + easeInTime + linearTime + easeOutTime;
            let startTime = null;

            function scrollRightStep(timestamp) {
                if (isChangelogManualScrolling || !document.getElementById('easter-credit-container') || !isAutoscrollAllowed()) return;
                if (!startTime) startTime = timestamp;
                const elapsed = timestamp - startTime;

                let curPos = startPos;

                if (elapsed < initialDelay) {
                    curPos = startPos;
                } else if (elapsed < initialDelay + easeInTime) {
                    const t = (elapsed - initialDelay) / easeInTime;
                    curPos = startPos + (distEaseIn * (t * t));
                } else if (elapsed < initialDelay + easeInTime + linearTime) {
                    const t = (elapsed - initialDelay - easeInTime) / 1000;
                    curPos = startPos + distEaseIn + (t * targetLinearSpeed);
                } else if (elapsed < totalDuration) {
                    const t = (elapsed - initialDelay - easeInTime - linearTime) / easeOutTime;
                    const easeFactor = 1 - Math.pow(1 - t, 3);
                    curPos = startPos + distEaseIn + linearDist + (distEaseOut * easeFactor);
                } else {
                    curPos = maxScroll;
                }

                panel.scrollLeft = Math.min(maxScroll, curPos);

                if (elapsed < totalDuration && panel.scrollLeft < maxScroll - 1) {
                    changelogHScrollAnimId = requestAnimationFrame(scrollRightStep);
                } else {
                    panel.scrollLeft = maxScroll;
                    changelogHScrollTimer = setTimeout(() => {
                        fastScrollHToLeft(panel, () => {
                            resumeChangelogHScroll();
                        });
                    }, 1000);
                }
            }

            changelogHScrollAnimId = requestAnimationFrame(scrollRightStep);
        }

        // FAST SCROLL DARI 1 KE 0 (MENTOK KIRI)
        function fastScrollHToLeft(panel, onComplete) {
            const startPos = panel.scrollLeft;
            if (startPos <= 0) {
                if (onComplete) onComplete();
                return;
            }

            const duration = 650;
            let startTime = null;
            const easeInOutQuad = t => t < 0.5 ? 2 * t * t : -1 + (4 - 2 * t) * t;

            function scrollLeftStep(timestamp) {
                if (!startTime) startTime = timestamp;
                const elapsed = timestamp - startTime;
                const t = Math.min(1, elapsed / duration);
                panel.scrollLeft = startPos * (1 - easeInOutQuad(t));

                if (t < 1 && panel.scrollLeft > 0) {
                    changelogHScrollAnimId = requestAnimationFrame(scrollLeftStep);
                } else {
                    panel.scrollLeft = 0;
                    if (onComplete) onComplete();
                }
            }
            changelogHScrollAnimId = requestAnimationFrame(scrollLeftStep);
        }
		
		document.addEventListener('click', function(e) {
            const githubBtn = e.target.closest('.github-link') || e.target.closest('.easter-github-clone');
            if (githubBtn && isEasterTransitioning) {
                e.preventDefault();
                e.stopImmediatePropagation();
            }
        }, true);

        function setupButtonListeners() {
            const btnDarkEl = document.getElementById('btn-mode-dark');
            const btnLightEl = document.getElementById('btn-mode-light');

            if (btnDarkEl) {
                btnDarkEl.addEventListener('click', (e) => {
                    e.stopPropagation();
                    setThemeMode('dark');
                });
            }

            if (btnLightEl) {
                btnLightEl.addEventListener('click', (e) => {
                    e.stopPropagation();
                    setThemeMode('light');
                });
            }

            const headerTitleContainer = document.querySelector('.header-title');
            if (headerTitleContainer) {
                headerTitleContainer.addEventListener('wheel', function(e) {
                    e.preventDefault();
                    e.stopPropagation();
                }, { passive: false });
            }

            const titleEl = document.getElementById('header-app-title');
            if (titleEl) {
                const savedTitle = safeStorage.getItem(`rizkyby_${windowId}_app_title`);
                if (savedTitle) {
                    titleEl.textContent = savedTitle;
                }

                titleEl.addEventListener('mousedown', function(e) { 
                    if (this.contentEditable !== "true") {
                        e.stopPropagation(); 
                    }
                });

                titleEl.addEventListener('click', function(e) {
                    e.stopPropagation();
                    this.contentEditable = "true";
                    this.style.cursor = "text";
                    this.style.userSelect = "text";
                    this.style.webkitUserSelect = "text";
                    this.dataset.scrolling = "paused";
                    if (globalTooltip) globalTooltip.style.opacity = '0';
                    this.focus();

                    setTimeout(() => {
                        const range = document.createRange();
                        const sel = window.getSelection();
                        range.selectNodeContents(this);
                        sel.removeAllRanges();
                        sel.addRange(range);
                    }, 10);
                });

                titleEl.addEventListener('keydown', function(e) {
                    if (e.key === 'Enter') {
                        e.preventDefault();
                        this.blur();
                    }
                });

                titleEl.addEventListener('blur', function() {
                    this.contentEditable = "false";
                    this.style.cursor = "default";
                    this.style.userSelect = "none";
                    this.style.webkitUserSelect = "none";
                    this.dataset.scrolling = "";
                    
                    const sel = window.getSelection();
                    if (sel) sel.removeAllRanges();

                    const newName = this.textContent.trim();
                    if (newName) {
                        safeStorage.setItem(`rizkyby_${windowId}_app_title`, newName);
                        saveConfig({ app_title: newName });
                    }
                });

                // Klik kanan pada judul: Beralih antara Dashboard dan About Panel dengan Animasi Out 2 arah
                // Pasang langsung menggunakan capture phase agar tidak ditelan elemen lain
                titleEl.addEventListener('contextmenu', function(e) {
                    // KUNCI: Abaikan jika event terpicu dari dalam kloningan frozen
                    if (e.target.closest('#frozen-about-panel-reference')) return;

                    e.preventDefault();
                    e.stopPropagation();

                    if (this.contentEditable === "true") return;

                    const dashboardEl = document.querySelector('.dashboard');
                    const aboutEl = document.getElementById('about-panel');
                    const isCurrentlyAbout = document.body.classList.contains('show-about-panel');

                    if (!isCurrentlyAbout) {
                        if (typeof updateEasterEggContent === 'function') updateEasterEggContent(true);
                        if (dashboardEl) {
                            dashboardEl.classList.remove('cards-animate-in');
                            dashboardEl.classList.add('cards-hidden');
                        }

                        setTimeout(() => {
                            if (dashboardEl) dashboardEl.style.display = 'none';
                            document.body.classList.add('show-about-panel');
                            if (aboutEl) {
                                aboutEl.scrollTop = 0;
                                aboutEl.classList.remove('about-hidden');
                            }
                            startAboutCreditsScroll();
                            // Jaga agar frozen credit tetap tersinkron tanpa di-hide
                            if (typeof syncFrozenCreditClone === 'function') syncFrozenCreditClone();
                        }, 300);
                    } else {
                        if (typeof updateEasterEggContent === 'function') updateEasterEggContent(false);
                        stopAboutAutoScroll();
                        if (aboutEl) aboutEl.classList.add('about-hidden');

                        setTimeout(() => {
                        document.body.classList.remove('show-about-panel');
                        if (aboutEl) {
                            aboutEl.classList.remove('about-hidden');
                            aboutEl.scrollTop = 0;
                        }

                        if (dashboardEl) {
                            dashboardEl.style.display = 'grid';
                            dashboardEl.classList.remove('cards-hidden');
                            dashboardEl.classList.add('cards-animate-in');
                            setTimeout(() => {
                                dashboardEl.classList.remove('cards-animate-in');
                            }, 400);
                        }

                        // JANGAN ADA instruksi .remove() atau styling pada frozenCreditClone di sini.
                        // Biarkan dia diam & berdiri di koordinatnya.
                    }, 300);
                    }
                }, true);
            }
        }

        if (document.readyState === 'loading') {
            document.addEventListener('DOMContentLoaded', () => {
                setupButtonListeners();
                initFrozenCreditClone();
            });
        } else {
            setupButtonListeners();
            initFrozenCreditClone();
        }

