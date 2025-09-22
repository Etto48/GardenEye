import { createRouter, createWebHistory } from 'vue-router'
import HomeView from '../views/DashboardView.vue'
import HistoryView from '../views/HistoryView.vue'
import GlobalSettingsView from '../views/GlobalSettingsView.vue'
import SensorSettingsView from '../views/SensorSettingsView.vue'
import NotFoundView from '../views/NotFoundView.vue'

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/',
      name: 'home',
      component: HomeView,
    },
    {
      path: '/history',
      name: 'history',
      component: HistoryView,
    },
    {
      path: '/sensor-settings',
      name: 'sensor-settings',
      component: SensorSettingsView,
    },
    {
      path: '/settings',
      name: 'settings',
      component: GlobalSettingsView,
    },
    {
      path: '/:pathMatch(.*)*',
      name: 'not-found',
      component: NotFoundView,
    },
  ],
})

export default router
