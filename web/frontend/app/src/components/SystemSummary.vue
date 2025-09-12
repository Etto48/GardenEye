<script setup lang="ts">
import type { Info } from '@/types/info';
import SummaryCard from './SummaryCard.vue';
import { onMounted, ref } from 'vue'

const loading = ref(true)
const error = ref<string | null>(null)
const info = ref<Info[]>([])

const fetchInfo = async () => {
    try {
        loading.value = true
        error.value = null
        
        const response = await fetch('/api/info')
        
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`)
        }
        
        const data = await response.json()
        info.value = data
    } catch (err) {
        error.value = err instanceof Error ? err.message : 'Failed to fetch sensors'
        console.error('Error fetching sensors:', err)
    } finally {
        loading.value = false
    }
}

onMounted(() => {
    fetchInfo()
})

// Expose the fetchInfo function to parent component
defineExpose({
    fetchInfo,
    loading
})

</script>

<template>
    <div class="flex flex-row justify-center flex-wrap w-full gap-md mb-3">
        <SummaryCard v-for="item in info" :key="item.title ? item.title : item.content" :title="item.title" :content="item.content" :level="item.level" />
    </div>
</template>

<style scoped>
</style>