<script setup lang="ts">
defineProps<{
    title?: string,
    content: string,
    type: 'info' | 'warning' | 'error'
}>();
</script>

<template>
    <div class="card summary-card hover-lift animate-fade-in" :class="`summary-card-${type}`">
        <h3 v-if="title != null">{{ title }}</h3>
        <p>{{ content }}</p>
    </div>
</template>

<style scoped>
.summary-card {
    padding: 1rem;
    border-radius: 0.5rem;
    box-shadow: var(--shadow);
    transition: transform 0.2s, box-shadow 0.2s;
    text-align: center;
    align-content: center;
    min-width: 150px;
    min-height: 150px;
    flex: 1;
}

.summary-card h3 {
    margin: 0 0 0.5rem 0;
    font-size: 1.25rem;
    color: var(--color-background-mute);
}

.summary-card p {
    margin: 0;
    font-size: 2rem;
    font-weight: bold;
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