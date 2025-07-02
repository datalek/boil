export type Either<L, R> =
  | { readonly type: 'left'; readonly value: L }
  | { readonly type: 'right'; readonly value: R };

export const right = <L = unknown, R = unknown>(value: R): Either<L, R> => ({
  type: 'right',
  value,
});

export const left = <L = unknown, R = unknown>(value: L): Either<L, R> => ({
  type: 'left',
  value,
});

export const tryCatch = async <L, R>(
  fn: () => Promise<R> | R,
  onError: (e: unknown) => L = (_) => _ as L,
): Promise<Either<L, R>> => {
  // eslint-disable-next-line functional/no-try-statements
  try {
    const value = await fn();
    return { type: 'right', value } as const;
  } catch (error) {
    return { type: 'left', value: onError(error) } as const;
  }
};
