// 格式化日期时间字符串
export function formatDateTime(input: string | undefined | null, onlyDate: boolean = false): string {
    if (!input) return '未知时间';

    const date = new Date(input);

    // 防止非法时间
    if (isNaN(date.getTime())) return '未知时间';

    const pad = (n: number) => n.toString().padStart(2, '0');

    const y = date.getFullYear();
    const m = pad(date.getMonth() + 1);
    const d = pad(date.getDate());

    if (onlyDate) {
        return `${y}-${m}-${d}`;
    }

    const h = pad(date.getHours());
    const min = pad(date.getMinutes());
    const s = pad(date.getSeconds());

    return `${y}-${m}-${d} ${h}:${min}:${s}`;
}

// 归属地解析
export const formatIpLocation = (location?: string | null): string => {
    if (!location) return '未知地区';

    if (location.includes('内网IP')) {
        return '内网IP';
    }

    const parts = location.split('|');

    // 常见格式：中国|0|湖南省|长沙市|电信
    const country = parts[0];
    const province = parts[2];
    const city = parts[3];
    const carrier = parts[4];

    const result = [province, city, carrier]
        .filter(item => item && item !== '0' && item !== '内网IP')
        .join(' ');

    return result || country || '未知地区';
};

// 安全过滤 HTML，返回是否包含不安全内容和清理后的 HTML
const FORBIDDEN_TAGS = new Set([
    'script','style','link','meta','object','embed','frame','frameset',
    'form','input','button','textarea','select'
]);
const SAFE_PROTOCOL = /^(https?:|\/|mailto:)/i;
const DANGEROUS_STYLE = /(expression|url\s*\(|javascript:)/i;
export function ValidateHtml(html: string) {
    const parser = new DOMParser();
    const doc = parser.parseFromString(html, 'text/html');

    let valid = true;

    const walk = (node: Node) => {
        if (node.nodeType !== 1) return; // 只处理 Element

        const el = node as Element;
        const tag = el.tagName.toLowerCase();

        if (FORBIDDEN_TAGS.has(tag)) {
            valid = false;
            el.remove();
            return;
        }

        [...el.attributes].forEach(attr => {
            const name = attr.name.toLowerCase();
            const value = attr.value.trim();

            if (name.startsWith('on')) {
                el.removeAttribute(name);
                valid = false;
                return;
            }

            if ((name === 'href' || name === 'src') && !SAFE_PROTOCOL.test(value)) {
                el.removeAttribute(name);
                valid = false;
                return;
            }

            // style 安全过滤
            if (name === 'style' && DANGEROUS_STYLE.test(value)) {
                el.removeAttribute('style');
                valid = false;
            }
        });

        // iframe 更严格
        if (tag === 'iframe') {
            const src = el.getAttribute('src') || '';

            if (!SAFE_PROTOCOL.test(src)) {
                el.remove();
                valid = false;
                return;
            }

            el.setAttribute('sandbox', '');
            el.setAttribute('loading', 'lazy');
            el.setAttribute('referrerpolicy', 'no-referrer');
        }

        [...el.childNodes].forEach(walk);
    };

    [...doc.body.childNodes].forEach(walk);

    return {
        valid,
        clean: doc.body.innerHTML
    };
}
