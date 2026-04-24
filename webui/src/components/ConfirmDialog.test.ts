import { describe, it, expect, beforeEach, vi } from 'vitest'
import { render, screen, fireEvent } from '@testing-library/svelte'
import ConfirmDialog from './ConfirmDialog.svelte'

describe('ConfirmDialog', () => {
  beforeEach(() => {
    try { localStorage.clear() } catch { /* jsdom storage not always available */ }
    vi.clearAllMocks()
  })

  it('renders when open=true', () => {
    const { container } = render(ConfirmDialog, {
      props: {
        open: true,
        title: 'Test Title',
        message: 'Test message'
      }
    })
    expect(screen.getByText('Test Title')).toBeInTheDocument()
    expect(screen.getByText('Test message')).toBeInTheDocument()
  })

  it('does not render when open=false', () => {
    const { container } = render(ConfirmDialog, {
      props: {
        open: false,
        title: 'Hidden Title',
        message: 'Hidden message'
      }
    })
    expect(screen.queryByText('Hidden Title')).not.toBeInTheDocument()
  })

  // skipped: vitest 4 + jsdom 29 localStorage shim lacks getItem/clear (TA-219 follow-up)
  it.skip('writes to localStorage when skipKey is set and checkbox is checked', async () => {
    render(ConfirmDialog, {
      props: {
        open: true,
        title: 'Confirm',
        message: 'Test',
        skipKey: 'test-skip'
      }
    })

    const checkbox = screen.getByRole('checkbox') as HTMLInputElement
    await fireEvent.click(checkbox)

    const buttons = screen.getAllByRole('button')
    const confirmBtn = buttons[buttons.length - 1]
    await fireEvent.click(confirmBtn)

    expect(localStorage.getItem('test-skip')).toBe('1')
  })

  it.skip('does not write to localStorage if checkbox unchecked', async () => {
    render(ConfirmDialog, {
      props: {
        open: true,
        title: 'Confirm',
        message: 'Test',
        skipKey: 'test-skip'
      }
    })

    const buttons = screen.getAllByRole('button')
    const confirmBtn = buttons[buttons.length - 1]
    await fireEvent.click(confirmBtn)

    expect(localStorage.getItem('test-skip')).toBeNull()
  })

  it('fires confirm event on confirm button click', async () => {
    let confirmFired = false
    const { component } = render(ConfirmDialog, {
      props: {
        open: true,
        title: 'Confirm',
        message: 'Test'
      }
    })

    component.$on('confirm', () => {
      confirmFired = true
    })

    const buttons = screen.getAllByRole('button')
    const confirmBtn = buttons[buttons.length - 1] // Last button is Confirm
    await fireEvent.click(confirmBtn)

    expect(confirmFired).toBe(true)
  })

  it('fires cancel event on cancel button click', async () => {
    let cancelFired = false
    const { component } = render(ConfirmDialog, {
      props: {
        open: true,
        title: 'Confirm',
        message: 'Test'
      }
    })

    component.$on('cancel', () => {
      cancelFired = true
    })

    const buttons = screen.getAllByRole('button')
    const cancelBtn = buttons[0] // First button is Cancel
    await fireEvent.click(cancelBtn)

    expect(cancelFired).toBe(true)
  })

  it('hides skipKey checkbox when not provided', () => {
    render(ConfirmDialog, {
      props: {
        open: true,
        title: 'Confirm',
        message: 'Test'
      }
    })

    expect(screen.queryByText("Don't show this again")).not.toBeInTheDocument()
  })
})
