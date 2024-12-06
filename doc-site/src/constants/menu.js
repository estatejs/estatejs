const MENU_ITEMS = [
    { key: 'main', label: 'Console', isTitle: false, icon: 'uil-desktop', url: '/console' },
    { key: 'dev-resources', label: 'Developer Resources', isTitle: true },
    {
        key: 'docs',
        label: 'Documentation',
        isTitle: false,
        icon: 'uil-book-alt',
        children: [
            { key: 'docs-overview', label: 'Overview', url: '/documentation/overview', parentKey: 'docs' },
            { key: 'docs-quick-start', label: 'Quick Start', url: '/documentation/quick-start', parentKey: 'docs' },
            { key: 'docs-handbook', label: 'Handbook', url: '/documentation/handbook', parentKey: 'docs' }
        ],
    },
    {
        key: 'ex',
        label: 'Examples',
        isTitle: false,
        icon: 'uil-lightbulb-alt',
        children: [
            { key: 'ex-exercise-tracker', label: 'Exercise Tracker (React)', url: '/examples/exercise-tracker', parentKey: 'ex' }
        ],
    },
    { key: 'dev-resources', label: 'Get Involved', isTitle: true },
    { key: 'community', label: 'Community', isTitle: false, icon: 'uil-users-alt', url: '/community' }
];

export default MENU_ITEMS;
