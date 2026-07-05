import { useEffect, useRef } from 'react';
import type { RefObject } from 'react';

const FOCUSABLE =
  'a[href],button:not([disabled]),textarea:not([disabled]),input:not([disabled]),select:not([disabled]),[tabindex]:not([tabindex="-1"])';

/**
 * Accessibility behaviour for a modal dialog, applied to the dialog container ref:
 * - moves focus into the dialog on open (first focusable, else the container)
 * - traps Tab / Shift+Tab within the dialog
 * - closes on Escape
 * - restores focus to the previously-focused element on close
 * - locks body scroll while open
 *
 * The caller keeps its own markup; it only needs to attach `ref` to the dialog
 * container and set role="dialog" aria-modal="true" aria-labelledby={...}.
 */
export function useModalA11y(
  isOpen: boolean,
  onClose: () => void,
  dialogRef: RefObject<HTMLElement | null>,
  options: { disableClose?: boolean } = {}
) {
  const disableClose = options.disableClose ?? false;
  // Keep the latest onClose/disableClose without re-running the open effect.
  const onCloseRef = useRef(onClose);
  onCloseRef.current = onClose;
  const disableCloseRef = useRef(disableClose);
  disableCloseRef.current = disableClose;

  useEffect(() => {
    if (!isOpen) return;
    const node = dialogRef.current;
    const previouslyFocused = document.activeElement as HTMLElement | null;

    const focusables = (): HTMLElement[] =>
      node
        ? Array.from(node.querySelectorAll<HTMLElement>(FOCUSABLE)).filter(
            (el) => el.offsetParent !== null || el === document.activeElement
          )
        : [];

    (focusables()[0] ?? node)?.focus();

    const onKeyDown = (e: KeyboardEvent) => {
      if (e.key === 'Escape') {
        if (!disableCloseRef.current) {
          e.stopPropagation();
          onCloseRef.current();
        }
        return;
      }
      if (e.key !== 'Tab') return;
      const els = focusables();
      if (els.length === 0) {
        e.preventDefault();
        node?.focus();
        return;
      }
      const first = els[0];
      const last = els[els.length - 1];
      if (e.shiftKey && document.activeElement === first) {
        e.preventDefault();
        last.focus();
      } else if (!e.shiftKey && document.activeElement === last) {
        e.preventDefault();
        first.focus();
      }
    };

    document.addEventListener('keydown', onKeyDown, true);
    const prevOverflow = document.body.style.overflow;
    document.body.style.overflow = 'hidden';

    return () => {
      document.removeEventListener('keydown', onKeyDown, true);
      document.body.style.overflow = prevOverflow;
      previouslyFocused?.focus?.();
    };
  }, [isOpen, dialogRef]);
}
