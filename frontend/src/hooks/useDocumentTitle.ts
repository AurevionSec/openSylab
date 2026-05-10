import { useEffect, useRef, useState, useCallback } from 'react';

/**
 * OPENSYLAB TITLE ARCHITECTURE
 * =============================
 * Formula: [Specific Context] | [Module] | OpenSylab
 *
 * Examples:
 * - Dashboard | OpenSylab
 * - S045 - Edit Sample | OpenSylab
 * - * S045 - Edit Sample | OpenSylab (dirty state)
 * - New Order | OpenSylab
 */

interface DocumentTitleOptions {
  /** Specific context (e.g., "S045", "Dashboard") */
  context?: string;
  /** Module name (e.g., "Samples", "Orders") */
  module?: string;
  /** Action (e.g., "Edit Sample", "View Order") */
  action?: string;
  /** Dirty state - adds asterisk prefix */
  isDirty?: boolean;
}

/**
 * Hook to manage document title with OpenSylab architecture
 *
 * @example
 * // Global overview
 * useDocumentTitle({ module: 'Dashboard' });
 * // Result: "Dashboard | OpenSylab"
 *
 * @example
 * // Specific entity
 * useDocumentTitle({ context: 'S045', action: 'Edit Sample' });
 * // Result: "S045 - Edit Sample | OpenSylab"
 *
 * @example
 * // Dirty state
 * useDocumentTitle({ context: 'S045', action: 'Edit Sample', isDirty: true });
 * // Result: "* S045 - Edit Sample | OpenSylab"
 *
 * @example
 * // Creation process
 * useDocumentTitle({ action: 'New Sample' });
 * // Result: "New Sample | OpenSylab"
 */
export const useDocumentTitle = (options: DocumentTitleOptions) => {
  const { context, module, action, isDirty = false } = options;

  useEffect(() => {
    const parts: string[] = [];

    // Build the title string
    if (context && action) {
      // Specific entity: "S045 - Edit Sample"
      parts.push(`${context} - ${action}`);
    } else if (action) {
      // Creation or action without context: "New Sample"
      parts.push(action);
    } else if (module) {
      // Module overview: "Samples"
      parts.push(module);
    }

    // Add dirty state indicator
    const titlePart = parts.join(' ');
    const fullTitle = titlePart
      ? `${isDirty ? '* ' : ''}${titlePart} | OpenSylab`
      : 'OpenSylab';

    document.title = fullTitle;

    // Cleanup: restore default title on unmount
    return () => {
      document.title = 'OpenSylab';
    };
  }, [context, module, action, isDirty]);
};

/**
 * Hook to track dirty state (unsaved changes) in forms
 * Returns isDirty flag and control functions
 */
export const useDirtyState = () => {
  const [isDirty, setIsDirtyState] = useState(false);
  const setDirty = useCallback(() => setIsDirtyState(true), []);
  const setClean = useCallback(() => setIsDirtyState(false), []);
  return { isDirty, setDirty, setClean };
};
