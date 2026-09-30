import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';

export default defineConfig({
  site: 'https://harvyliu.github.io/OSFS',
  base: '/OSFS',
  integrations: [
    starlight({
      title: 'OS From Scratch',
      description: 'College OS, rewritten for beginners. Concept first, hands-on second.',
      social: { github: 'https://github.com/' },
      sidebar: [
        { label: 'Start here', link: '/' },
        { label: 'Roadmap', link: '/roadmap/' },
        { label: 'Glossary', link: '/glossary/' },
        { label: 'Lessons', autogenerate: { directory: 'lessons' } },
      ],
      customCss: ['./src/styles/custom.css'],
    }),
  ],
});
