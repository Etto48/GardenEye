<script setup lang="ts">
import { RouterLink, RouterView } from 'vue-router'
</script>

<template>
    <header class="app-header fixed top-0 left-0 right-0 flex items-center px-2 transition-colors">
        <img alt="GardenEye logo" class="logo" src="@/assets/favicon.svg" />
        <h1 class="title">GardenEye</h1>
        <nav class="flex-1 text-center">
            <RouterLink to="/">Dashboard</RouterLink>
        </nav>
    </header>
    
    <main>
        <RouterView />
    </main>
</template>

<style scoped>
.app-header {
    height: 80px;
    background-color: var(--color-background);
    border-bottom: 1px solid var(--color-border);
    z-index: 1000;
    box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
}

.app-header .logo {
    height: 40px;
    width: auto;
    margin: 0;
}

.app-header .title {
    font-weight: bold;
    margin-left: 10px;
}

.app-header nav {
    font-size: 12px;
    margin: 0;
    position: relative;
    top: 2px;
}

.app-header nav a.router-link-exact-active {
    color: var(--color-text);
}

.app-header nav a.router-link-exact-active:hover {
    background-color: transparent;
}

.app-header nav a {
    display: inline-block;
    margin: 1rem;
    border-left: 1px solid var(--color-border);
}

.app-header nav a:first-of-type {
    border: 0;
}

@media (min-width: 1024px) {
    .app-header {
        padding: 0 2rem;
    }
    
    .app-header nav {
        text-align: left;
        font-size: 1rem;
    }
}
</style>
