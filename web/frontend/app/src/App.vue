<script setup lang="ts">
import { RouterLink, RouterView } from 'vue-router'
import { ref, onMounted, onUnmounted } from 'vue'

const isMobileMenuOpen = ref(false)
const isMobile = ref(false)

const toggleMobileMenu = () => {
  isMobileMenuOpen.value = !isMobileMenuOpen.value
}

const closeMobileMenu = () => {
  isMobileMenuOpen.value = false
}

const checkScreenSize = () => {
  isMobile.value = window.innerWidth < 768
  if (!isMobile.value) {
    isMobileMenuOpen.value = false
  }
}

const handleEscapeKey = (event: KeyboardEvent) => {
  if (event.key === 'Escape') {
    closeMobileMenu()
  }
}

onMounted(() => {
  checkScreenSize()
  window.addEventListener('resize', checkScreenSize)
  document.addEventListener('keydown', handleEscapeKey)
})

onUnmounted(() => {
  window.removeEventListener('resize', checkScreenSize)
  document.removeEventListener('keydown', handleEscapeKey)
})
</script>

<template>
    <header class="app-header sticky flex items-center px-2 transition-colors">
        <div class="header-left flex items-center">
            <img alt="GardenEye logo" class="logo" src="/favicon.svg" />
            <h1 class="title">GardenEye</h1>
        </div>
        
        <!-- Navigation -->
        <nav :class="{ 'open': isMobileMenuOpen }">
            <RouterLink to="/" @click="closeMobileMenu"><i class="bi bi-house"></i>Dashboard</RouterLink>
            <RouterLink to="/settings" @click="closeMobileMenu"><i class="bi bi-gear"></i>Settings</RouterLink>
        </nav>
        
        <!-- Mobile Burger Menu Button -->
        <button 
            class="mobile-menu-button"
            @click="toggleMobileMenu"
            :aria-expanded="isMobileMenuOpen"
            aria-label="Toggle navigation menu"
        >
            <div class="burger-icon" :class="{ 'open': isMobileMenuOpen }">
                <span></span>
                <span></span>
                <span></span>
            </div>
        </button>
    </header>
    
    <!-- Mobile Navigation Overlay -->
    <div 
        class="mobile-nav-overlay" 
        :class="{ 'open': isMobileMenuOpen }"
        @click="closeMobileMenu"
    ></div>
    
    <main>
        <RouterView />
    </main>
</template>

<style scoped>

.app-header {
    height: 80px;
    width: 100%;
    top: 0;
    left: 0;
    right: 0;
    background-color: var(--color-background);
    border-bottom: 1px solid var(--color-border);
    z-index: 1000;
    box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
    padding: 0 2rem;
}

.app-header .logo {
    height: 40px;
    width: auto;
    margin: 0;
}

.app-header .title {
    font-weight: bold;
    margin: 0.5rem 1rem;
}

/* Navigation */
nav {
    margin: 0;
    position: relative;
    top: 2px;
    text-align: center;
    font-size: 1rem;
}

nav a.router-link-exact-active {
    color: var(--color-text);
}

nav a.router-link-exact-active:hover {
    background-color: transparent;
}

nav a {
    display: inline-block;
    margin: 0.5rem;
}

nav a:first-of-type {
    border: 0;
}

nav a i {
    margin-right: 0.5rem;
    color: inherit;
}

/* Mobile Menu Button */
.mobile-menu-button {
    display: none;
    background: none;
    border: none;
    cursor: pointer;
    padding: 0;
    width: 30px;
    height: 24px;
    position: relative;
    z-index: 1001;
}

/* Burger Icon Animation */
.burger-icon {
    position: relative;
    width: 30px;
    height: 24px;
}

.burger-icon span {
    display: block;
    position: absolute;
    height: 3px;
    width: 100%;
    background-color: var(--color-text);
    border-radius: 3px;
    opacity: 1;
    left: 0;
    transform: rotate(0deg);
    transition: 0.25s ease-in-out;
}

.burger-icon span:nth-child(1) {
    top: 0px;
}

.burger-icon span:nth-child(2) {
    top: 10px;
}

.burger-icon span:nth-child(3) {
    top: 20px;
}

/* Burger Icon Animation - Open State */
.burger-icon.open span:nth-child(1) {
    top: 10px;
    transform: rotate(135deg);
}

.burger-icon.open span:nth-child(2) {
    opacity: 0;
    left: -60px;
}

.burger-icon.open span:nth-child(3) {
    top: 10px;
    transform: rotate(-135deg);
}

/* Mobile Navigation Overlay */
.mobile-nav-overlay {
    position: fixed;
    top: 0;
    left: 0;
    width: 100%;
    height: 100vh;
    background-color: rgba(0, 0, 0, 0.5);
    z-index: 999;
    opacity: 0;
    visibility: hidden;
    transition: opacity 0.3s ease, visibility 0.3s ease;
}

.mobile-nav-overlay.open {
    opacity: 1;
    visibility: visible;
}

/* Mobile Responsive Design */
@media (max-width: 767px) {
    nav {
        border-radius: 0 0 0 0.5rem;
        position: fixed;
        top: 80px;
        right: 0;
        width: 250px;
        background-color: var(--color-background);
        border-left: 1px solid var(--color-border);
        border-bottom: 1px solid var(--color-border);
        box-shadow: -2px 2px 8px rgba(0, 0, 0, 0.1);
        transform: translateX(100%);
        transition: transform 0.3s ease;
        padding: 1rem 0;
        z-index: 1000;
        text-align: left;
    }
    
    nav.open {
        transform: translateX(0);
    }
    
    nav a {
        display: block;
        padding: 1rem 2rem;
        text-decoration: none;
        color: var(--color-text);
        border-bottom: 1px solid var(--color-border);
        transition: background-color 0.2s ease;
        margin: 0;
    }
    
    nav a:hover {
        background-color: var(--color-background-soft);
    }
    
    nav a.router-link-exact-active {
        background-color: var(--color-background-mute);
        color: var(--color-accent-1);
    }
    
    nav a:last-child {
        border-bottom: none;
    }
    
    .mobile-menu-button {
        display: block;
    }

    .header-left {
        flex: 1;
    }
}

/* Tablet and up - keep desktop nav visible */
@media (min-width: 768px) {
    .mobile-menu-button {
        display: none;
    }
    
    .mobile-nav-overlay {
        display: none;
    }
}
</style>
