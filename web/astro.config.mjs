// @ts-check
import { defineConfig } from 'astro/config';

// https://astro.build/config
const pagesOrigin = process.env.GITHUB_PAGES_ORIGIN ?? 'https://jbvenuto.github.io';
const pagesBasePath = process.env.GITHUB_PAGES_BASE_PATH ?? '/';
const base = pagesBasePath || '/';
const homeRedirect = `${pagesBasePath.replace(/\/$/, '')}/` || '/';

export default defineConfig({
	site: pagesOrigin,
	base,
	redirects: {
		'/Home': homeRedirect,
	},
});
