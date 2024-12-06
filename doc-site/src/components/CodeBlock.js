import React, {useEffect} from "react";

import Prism from "prismjs";
import 'prismjs/plugins/toolbar/prism-toolbar';
import 'prismjs/plugins/copy-to-clipboard/prism-copy-to-clipboard';
import "../assets/css/prism-vsc-dark-plus.css";

export default function CodeBlock({code, language}) {
    useEffect(() => {
        Prism.highlightAll();
    }, []);
    return (
        <div className="Code">
      <pre>
        <code className={`language-${language}`}>{code}</code>
      </pre>
        </div>
    );
}