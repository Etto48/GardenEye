<script setup lang="ts">
defineProps<{
    title?: string,
    content: string,
    level: 'info' | 'warning' | 'error'
}>();
</script>

<template>
    <div class="card summary-card hover-lift animate-fade-in text-center align-content-center min-w-150 min-h-150 flex-1" :class="`summary-card-${level}`">
        <h3 v-if="title != null" class="m-0 mb-1 text-xl">{{ title }}</h3>
        <p class="m-0 text-3xl font-bold">{{ content }}</p>
    </div>
</template>

<style scoped>
.summary-card {
    transition: transform 0.2s, box-shadow 0.2s;
}

.summary-card h3 {
    color: var(--color-background-mute);
}

.summary-card p:first-child {
    font-size: 1.5rem;
}

.summary-card-info {
    background: linear-gradient(135deg, var(--color-info-1), var(--color-info-2));
    color: var(--color-background);
}

.summary-card-warning {
    background: linear-gradient(135deg, var(--color-warning-1), var(--color-warning-2));
    color: var(--color-background);
}

.summary-card-error {
    background: linear-gradient(135deg, var(--color-error-1), var(--color-error-2));
    color: var(--color-background);
}
</style>