<script setup lang="ts">
import SummaryCard from './SummaryCard.vue';

</script>

<template>
    <div class="system-summary flex flex-row">
        <SummaryCard title="info" content="0" type="info" />
        <SummaryCard title="warning" content="1" type="warning"/>
        <SummaryCard title="error" content="2" type="error"/>
        <SummaryCard content="info" type="info" />
        <SummaryCard content="warning" type="warning"/>
        <SummaryCard content="error" type="error"/>
    </div>
</template>

<style scoped>
.system-summary {
    gap: 1rem;
    margin-bottom: 1rem;
    justify-content: center;
    flex-wrap: wrap;
    width: 100%;
}
</style>